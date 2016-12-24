#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import unicode_literals, print_function
import re
from Queue import Queue
from pypeg2 import *
import ast
import os

"""
REQUIREMENTS:
pip install pypeg2
pip install islpy
"""



GRAMMAR = """
<definition> := [<annotations>] define [type] <name> = <expression>;
<assumption> := [<annotations>] assume <name>: <expression>;

<expression> :=   lambda <name>: <expression>. <expression>
				| pi <name>: <expression>. <expression>
				| [<annotations>] lambda rec <funcname> (<param>:<type-expression>): <returntype-expression>. <expression>
				| let <definition>... in <expression> end
				| <extract-expression> [<extract-expression>]+ [-> <expression>]
<extract-expression> := <atomic-expression> [ '[' <int> ']' ]+
<atomic-expression>  := Nat | '*' | Param | Var

<annotations> := <annotation>*
<annotation>  := @<name> "<text>"
"""



TEST = """
define Nat = Nat;
assume n1: Nat;
assume opPlus: Nat -> Nat -> Nat;

define test1 = let
		define x = n1;
		define y = n1;
	in opPlus (x, y) end;

define test2 = let in Nat end;

define test3 = lambda n:Nat. let
	define r1 = lambda rec (x:Nat): Nat. r2 x;
	define r2 = lambda rec (y:Nat): Nat. r1 n1;
in (r1 n, r2 n) end;

"""



class Scope:
	def __init__(self, parent=None):
		self.vars = {}
		self.depth = 0
		self.parent = parent # type: Scope

	def push_param(self, vname=None, vtype=None):
		scope = self.push()
		if vname:
			scope.vars[vname] = ('param', vtype, self.depth)
		return scope

	def push(self):
		scope = Scope(self)
		scope.depth = self.depth + 1
		return scope

	def add_definition(self, defname, deftype=None):
		self.vars[defname] = ('def', deftype, 0)

	def contains(self, name):
		return name in self.vars or (self.parent is not None and self.parent.contains(name))

	def __contains__(self, item):
		return self.contains(item)

	def __getitem__(self, item):
		if item in self.vars:
			return self.vars[item]
		if self.parent is not None:
			return self.parent[item]
		raise IndexError(item)


class CppCodeGen:
	def __init__(self):
		self.decls = []
		self.temp_varname_counter = 0
		self.unknown_identifiers = []
		self.pending_decls = []

	# Manage list of CPP lines

	def add_comment(self, comment):
		self.decls.append(('comment', comment))

	def add_decl(self, name, decl):
		self.decls.append(('decl', decl))
		# add pending decls if preconditions are fulfilled
		for (inst, depends) in self.pending_decls:
			depends.discard(name)

	def add_inst(self, inst):
		self.decls.append(('inst', inst))

	def add_blank(self):
		self.decls.append(('blank', ''))

	def add_depending(self, inst, depends_on):
		if len(depends_on) == 0:
			return self.add_inst(inst)
		self.pending_decls.append((inst, set(depends_on)))

	# add all "resolved" dependencies
	def progress_depending(self):
		for (inst, depends) in self.pending_decls:
			if len(depends) == 0:
				self.add_inst(inst)
		self.pending_decls = [(i, d) for i, d in self.pending_decls if len(d) > 0]

	# Unique variable names
	def get_temp_varname(self):
		self.temp_varname_counter += 1
		return 'lbl_tmp_'+str(self.temp_varname_counter)

	# Handling unknown identifiers
	def unknown_identifier(self, ident):
		self.unknown_identifiers.append(ident)

	def get_unknown_identifiers(self):
		return self.unknown_identifiers

	def set_unknown_identifiers_list(self, l):
		self.unknown_identifiers = l

	# Returning generated code and status

	def output(self, indent=''):
		return '\n'.join([indent + y for x, y in self.decls])

	def warnings(self):
		warnings = []
		for x in set(self.unknown_identifiers):
			warnings.append('[WARN] Unknown variable: '+str(x))
		for i, d in self.pending_decls:
			warnings.append('[WARN] Recursive declaration waiting for {} to appear'.format(', '.join(d)))
		return warnings


class AstCreator:
	def __init__(self):
		self.queue = Queue()
		self.seen = []  # set does not work, as nodes are unhashable / mutable
		self.after_creation = []

	def append(self, node, scope, callback):
		for n in self.seen:
			if n is node:
				return
		self.seen.append(node)
		self.queue.put((node, scope, callback))

	def run_after_creation(self, cb):
		self.after_creation.append(cb)

	def progress_all(self):
		while not self.queue.empty():
			node, scope, callback = self.queue.get()
			obj = node.to_ast(scope, self)
			callback(obj)
		for cb in self.after_creation:
			cb()




class ParserAstNode:
	"""
	Base class for parsing code. Please refer to the pypeg2 documentation.
	Each language feature is a subclass.
	"""
	def normalize(self):
		"""
		Removes all syntactic sugar from the AST.
		:return: the new representation of this node
		"""
		if isinstance(self, List):
			for i in xrange(len(self)):
				if isinstance(self[i], ParserAstNode):
					self[i] = self[i].normalize()
		for (k, v) in self.__dict__.items():
			if isinstance(v, ParserAstNode):
				self.__dict__[k] = v.normalize()
		return self

	def to_cpp(self, scope, codegen, opts={}):
		"""
		:param Scope scope:
		:param CppCodeGen codegen:
		:param dict opts:
		:return:
		"""
		raise Exception('NI')

	def to_ast(self, scope, astcreator):
		"""
		:param Scope scope:
		:param AstCreator astcreator:
		:return: ast.AstNode
		"""
		raise Exception('NI')


def cpp_string(x):
	return '"'+x.replace('"', r'\"')+'"'

def cpp_comment(x):
	return '\n'.join(['// '+y for y in x.split('\n')])





Expression = []
AtomicExpression = []
Keyword.regex = re.compile(r'\w+|\*')
Symbol.regex = re.compile(r'\w+|\*')
Symbol.check_keywords = True


class Annotation(str, ParserAstNode):
	grammar = '@', name(), blank, attr('content', re.compile(r'"([^"]|(\\"))*"')), endl
	valid_annotations = set(['guarantees', 'accepts'])

	def get_content(self):
		return self.content[1:-1].replace(r'\"', '"').replace(r'\\', '\\').replace(r'\n', '\n')

	def __repr__(self):
		return compose(self)[:-1].encode('utf-8')


class Annotations(List, ParserAstNode):
	grammar = maybe_some(Annotation),

	def assert_valid(self):
		invalid = [a.name for a in self if a.name not in Annotation.valid_annotations]
		if len(invalid) > 0:
			raise Exception('Invalid assertions: '+', '.join(invalid))

	def get_all(self, name):
		return [a.get_content() for a in self if a.name == name]

	def __contains__(self, item):
		for a in self:
			if a.name == item:
				return True
		return False

	def create_constraints(self, astnode):
		"""
		:param ast.GivenConstraint astnode:
		:return:
		"""
		accepted = ' and '.join(('('+c+')' for c in self.get_all('accepts')))
		possible = ' and '.join(('('+c+')' for c in self.get_all('guarantees')))
		astnode.create_constraints(None, accepted, possible)

	def __bool__(self):
		return len(self) > 0

	__nonzero__ = __bool__



class Identifier(str, ParserAstNode):
	grammar = name()

	def to_cpp(self, scope, codegen, opts={}):
		# check if var in scope
		if self.name not in scope:
			codegen.unknown_identifier(self.name)
			return str(self.name)
		# resolve var
		kind, vartype, acc = scope[self.name]
		if kind == 'param':
			return 'w.var(' + vartype.to_cpp(scope, codegen, {'accept_inline_tuple': True}) + ', ' + str(scope.depth - acc) + ', ' + cpp_string(self.name) + ')'
		return str(self.name)

	def __repr__(self):
		return 'Identifier('+repr(self.name)+')'

	def to_ast(self, scope, astcreator):
		if self.name not in scope:
			asm = ast.get_assumption(self.name)
			if asm:
				return asm
			raise Exception('Unknown value (TODO): '+repr(self.name))
		kind, vartype, acc = scope[self.name]
		return vartype


class Constant(Symbol, ParserAstNode):
	grammar = Enum(Symbol('*'), Symbol('Nat'))

	def compose(self, parser, attr_of):
		return self.name

	def to_cpp(self, scope, codegen, opts={}):
		if self.name == '*':
			return 'w.star()'
		elif self.name == 'Nat' and not scope.contains(self.name):
			return 'w.nat()'
		else:
			return self.name

	def to_ast(self, scope, astcreator):
		return ast.get_constant(self.name)


class Tupel(List, ParserAstNode):
	grammar = '(', csl(Expression), ')'

	def compose(self, parser, attr_of):
		return '('+', '.join([parser.compose(x, attr_of=self) for x in self])+')'

	def to_cpp(self, scope, codegen, opts={}):
		if len(self) == 1:
			return self[0].to_cpp(scope, codegen, opts)
		else:
			tuple = '{'+', '.join([element.to_cpp(scope, codegen) for element in self])+'}'
			if not 'accept_inline_tuple' in opts:
				tuple = 'w.tuple('+tuple+')'
			return tuple

	def to_ast(self, scope, astcreator):
		if len(self) == 1:
			return self[0].to_ast(scope, astcreator)
		else:
			return ast.Tupel([element.to_ast(scope, astcreator) for element in self])


class Lambda(Plain, ParserAstNode):
	grammar = ['lambda', '\\', 'λ'], blank, [name(), '_'], ':', attr('type', Expression), '.', blank, attr('body', Expression)

	def compose(self, parser, attr_of):
		return 'λ ' + self.name + ':' + parser.compose(self.type, attr_of=self) + '. ' + parser.compose(self.body, attr_of=self)

	def to_cpp(self, scope, codegen, opts={}):
		paramtype = self.type.to_cpp(scope, codegen, {'accept_inline_tuple':True})
		return 'w.lambda('+paramtype+', '+self.body.to_cpp(scope.push_param(self.name, self.type), codegen)+')'

	def to_ast(self, scope, astcreator):
		tp = self.type.to_ast(scope, astcreator)
		param = ast.ParamDef(self.name, tp)
		return ast.Lambda([param, self.body.to_ast(scope.push_param(self.name, param), astcreator)])


class LambdaRec(Plain, ParserAstNode):
	#lambda rec [name](param:type): returntype. expression
	grammar = attr('annotations', Annotations), ['lambda', '\\', 'λ'], blank, K('rec'), blank, optional(name()), \
			  '(', attr('param', [name(), '_']), ':', attr('type', Expression), ')', blank, \
			  ':', blank, attr('returntype', Expression), '.', blank, attr('body', Expression)

	def __init__(self):
		self.name = None

	def compose(self, parser, attr_of):
		result = 'λ rec ';
		if self.name:
			result += self.name + ' ';
		result += '(' + self.param.thing + ':' + parser.compose(self.type, attr_of=self) + ') : '
		result += parser.compose(self.returntype, attr_of=self) + '. ' + parser.compose(self.body, attr_of=self)
		return result

	def to_cpp(self, scope, codegen, opts={}):
		# declare variable without body
		paramtype = self.type.to_cpp(scope, codegen, {'accept_inline_tuple':True});
		returntype = self.returntype.to_cpp(scope, codegen);
		decl = 'w.lambda_rec(' + paramtype + ', ' + returntype;
		if self.name:
			decl += ', '+cpp_string(self.name)
		decl += ')'
		if 'cpp_name' in opts:
			varname = opts['cpp_name']
			result = decl
		else:
			varname = self.name or codegen.get_temp_varname()
			codegen.add_decl(varname, 'auto {} = {};'.format(varname, decl))
			scope.add_definition(varname, Pi.new('_', self.type, self.returntype))
			result = varname

		# body decl
		childscope = scope.push_param(self.param.thing, self.type)
		old_unknown = codegen.get_unknown_identifiers()
		codegen.set_unknown_identifiers_list([])
		body = self.body.to_cpp(childscope, codegen)
		codegen.add_depending('{}->set({});'.format(varname, body), codegen.get_unknown_identifiers())
		codegen.set_unknown_identifiers_list(old_unknown)
		return result

	def to_ast(self, scope, astcreator):
		self.annotations.assert_valid()
		tp = self.type.to_ast(scope, astcreator)
		param = ast.ParamDef(self.param.thing, tp)
		pscope = scope.push_param(self.param.thing, param)
		result = ast.LambdaNominal([param, self.returntype.to_ast(pscope, astcreator)])
		scope.add_definition(self.name, result)
		pscope.add_definition(self.name, result)
		if self.name:
			result.name = str(self.name)
		# enqueue body
		astcreator.append(self.body, pscope, lambda body: result.set_body(body))
		# add constraints (if any), after body creation
		if 'guarantees' in self.annotations or 'accepts' in self.annotations:
			astcreator.run_after_creation(lambda: self.annotations.create_constraints(result))
		return result


class Pi(Plain, ParserAstNode):
	grammar = ['pi', 'Π'], blank, [name(), '_'], ':', attr('type', Expression), '.', blank, attr('body', Expression)

	@staticmethod
	def new(pname, ptype, body):
		pi = Pi()
		pi.name = pname or '_'
		pi.type = ptype
		pi.body = body
		return pi

	def compose(self, parser, attr_of):
		# shorten "Π_:X. Y" to "X -> Y"
		if self.name == '_':
			return parser.compose(self.type, attr_of=self) + ' -> ' + parser.compose(self.body, attr_of=self)
		return 'Π ' + self.name + ':' + parser.compose(self.type, attr_of=self) + '. ' + parser.compose(self.body, attr_of=self)

	def to_cpp(self, scope, codegen, opts={}):
		paramtype = self.type.to_cpp(scope, codegen, {'accept_inline_tuple':True})
		return 'w.pi('+paramtype+', '+self.body.to_cpp(scope.push_param(self.name, self.type), codegen)+')'

	def to_ast(self, scope, astcreator):
		tp = self.type.to_ast(scope, astcreator)
		param = ast.ParamDef(self.name, tp)
		return ast.Pi([param, self.body.to_ast(scope.push_param(self.name, param), astcreator)])


class Extract(List, ParserAstNode):
	grammar = attr('base', AtomicExpression), maybe_some('[', re.compile(r'\d+'), ']')

	def to_cpp(self, scope, codegen, opts={}):
		if len(self) == 0:
			return self.base.to_cpp(scope, codegen, opts)
		result = self.base.to_cpp(scope, codegen)
		for index in self:
			result = 'w.extract('+result+', '+index+')'
		return result

	def to_ast(self, scope, astcreator):
		result = self.base.to_ast(scope, astcreator)
		for index in self:
			result = ast.Extract([result, index])
		return result


class SpecialFunction(Symbol, ParserAstNode):
	grammar = Enum(Symbol('sigma'), Symbol('tuple'))


class SpecialApp(Plain, ParserAstNode):
	grammar = attr('func', SpecialFunction), attr('param', Extract)

	def compose(self, parser, attr_of):
		return self.func+' '+parser.compose(self.param, attr_of=self)

	def to_cpp(self, scope, codegen, opts={}):
		if self.func.name == 'sigma':
			return 'w.sigma('+self.param.to_cpp(scope, codegen, {'accept_inline_tuple': True})+')'
		elif self.func.name == 'tuple':
			return 'w.tuple('+self.param.to_cpp(scope, codegen, {'accept_inline_tuple': True})+')'
		else:
			raise Exception("Unknown special function ("+str(self.func)+")")

	def to_ast(self, scope, astcreator):
		tpl = self.param.to_ast(scope, astcreator)
		if self.func.name == 'sigma':
			return ast.Sigma(tpl.ops)
		return tpl




class App(List, ParserAstNode):
	grammar = attr('func', [SpecialApp, Extract]), maybe_some(Extract), optional('->', attr('arrow', Expression))

	def __init__(self):
		self.arrow = None

	def normalize(self):
		# convert -> to pi
		if self.arrow is not None:
			pi = Pi.new('_', self, self.arrow)
			self.arrow = None
			return pi.normalize()
		# remove App without parameters
		#elif len(self) == 0:
		#	return self.func.normalize()
		else:
			return ParserAstNode.normalize(self)

	def to_expr(self):
		return self.func.to_expr() + ' '.join([x.to_expr() for x in self])

	def compose(self, parser, attr_of):
		result = parser.compose(self.func, attr_of=self)
		for param in self:
			result += ' ' + parser.compose(param, attr_of=self)
		return result

	def to_cpp(self, scope, codegen, opts={}):
		if len(self) == 0:
			return self.func.to_cpp(scope, codegen, opts)
		result = self.func.to_cpp(scope, codegen)
		for param in self:
			result = 'w.app('+result+', '+param.to_cpp(scope, codegen, {'accept_inline_tuple':True})+')'
		return result

	def __repr__(self):
		result = '('+repr(self.func);
		for param in self:
			result += '@'+repr(param)
		return result + ')'

	def to_ast(self, scope, astcreator):
		result = self.func.to_ast(scope, astcreator)
		for param in self:
			result = ast.App([result, param.to_ast(scope, astcreator)])
		return result


class InnerDefinition(Plain, ParserAstNode):
	grammar = attr('annotations', Annotations), K('define'), blank, flag('type', 'type'), name(), blank, '=', blank, attr("body", Expression), ';'

	def compose(self, parser, attr_of):
		return self.name+' := '+parser.compose(self.body)

	def to_cpp(self, scope, codegen, opts={}):
		codegen.add_comment(cpp_comment(compose(self, autoblank=True)))
		body = self.body.to_cpp(scope, codegen, {'cpp_name': self.name})
		scope.add_definition(self.name)
		codegen.add_decl(self.name, 'auto '+self.name+' = '+body+';')

	def to_ast(self, scope, astcreator):
		self.annotations.assert_valid()
		if self.annotations:
			if not hasattr(self.body, 'annotations'):
				raise Exception('Annotations not supported here')
			self.body.annotations = self.annotations
		body = self.body.to_ast(scope, astcreator)
		scope.add_definition(self.name, body)
		return [(str(self.name), body)]


class Definition(InnerDefinition):
	def to_cpp(self, scope, codegen, opts={}):
		InnerDefinition.to_cpp(self, scope, codegen)
		codegen.add_inst('printValue('+self.name+');')
		if not self.type:
			codegen.add_inst('printType(' + self.name + ');')
		codegen.progress_depending()
		codegen.add_blank()


class Assumption(Plain, ParserAstNode):
	grammar = attr('annotations', Annotations), K('assume'), blank, name(), ':', blank, attr("body", Expression), ';'

	def to_expr(self):
		return 'assume '+self.name+': '+self.body.to_expr()

	def to_cpp(self, scope, codegen, opts={}):
		codegen.add_comment(cpp_comment(compose(self, autoblank=True)))
		body = self.body.to_cpp(scope, codegen)
		scope.add_definition(self.name, self.body)
		codegen.add_decl(self.name, 'auto '+self.name+' = w.assume('+body+', '+cpp_string(self.name)+');')
		codegen.add_inst('printType(' + self.name + ');')
		codegen.progress_depending()
		codegen.add_blank()

	def to_ast(self, scope, astcreator):
		self.annotations.assert_valid()
		if ast.has_predefined_assumption(self.name):
			assumption = ast.get_assumption(self.name)
			# TODO assert type equality
		else:
			assumption = ast.Assume([self.name, self.body.to_ast(scope, astcreator)])
			if 'guarantees' in self.annotations or 'accepts' in self.annotations:
				self.annotations.create_constraints(assumption)
		scope.add_definition(self.name, assumption)
		return [(str(self.name), assumption)]


class Comment(str, ParserAstNode):
	# don't rely on this class! In many cases, comments are removed by regular expressions before they reach the parser.
	grammar = [comment_cpp, comment_sh, comment_c]

	def to_cpp(self, scope, codegen, opts={}):
		return ''

	def to_ast(self, scope, astcreator):
		return []



class LetIn(List, ParserAstNode):
	grammar = K('let'), blank, maybe_some([InnerDefinition, Comment]), blank, K('in'), blank, attr('expr', Expression), blank, K('end')

	def compose(self, parser, attr_of):
		return 'let {{ {} }} in {} end'.format(', '.join([parser.compose(d, attr_of=self) for d in self]), parser.compose(self.expr, attr_of=self))

	def to_cpp(self, scope, codegen, opts={}):
		inner_scope = scope.push()
		for definition in self:
			definition.to_cpp(inner_scope, codegen)
		return self.expr.to_cpp(inner_scope, codegen)

	def to_ast(self, scope, astcreator):
		inner_scope = scope.push()
		for definition in self:
			# this also adds the created nodes to inner_scope
			definition.to_ast(inner_scope, astcreator)
		return self.expr.to_ast(inner_scope, astcreator)



Expression.extend([LambdaRec, Lambda, Pi, LetIn, App])
AtomicExpression.extend([Tupel, Constant, Identifier])



class Program(List, ParserAstNode):
	grammar = -1, [Definition, Assumption, Comment]

	def to_cpp(self):
		scope = Scope()
		codegen = CppCodeGen()
		for x in self:
			x.to_cpp(scope, codegen)
		codegen.progress_depending()
		codegen.add_blank()
		for warn in codegen.warnings():
			print(warn)
		return codegen.output()

	def to_ast(self):
		"""
		:rtype: list[(str, ast.AstNode)]
		:return: A list containing (name, astnode) pairs for all assumptions and definitions in this program
		"""
		scope = Scope()
		astcreator = AstCreator()
		defs = []
		for x in self:
			defs += x.to_ast(scope, astcreator)
		astcreator.progress_all()
		return defs

	def defs_to_ast(self):
		"""
		:rtype: list[(str, ast.AstNode)]
		:return: A list containing (name, astnode) pairs for all definitions in this program
		"""
		scope = Scope()
		astcreator = AstCreator()
		defs = []
		for x in self:
			if isinstance(x, Definition):
				defs += x.to_ast(scope, astcreator)
			else:
				x.to_ast(scope, astcreator)
		astcreator.progress_all()
		return defs





# UTIL FUNCTIONS
def strip_comments(program):
	program = re.sub(re.compile("/\*.*?\*/", re.DOTALL), '', program)
	program = re.sub(re.compile("//.*?\n"), '', program)
	return program

def load_file_with_includes(filename, loaded_files=None):
	"""
	Load a code file from disk and progresses all its #include directives, possibly loading additional code files
	:param str filename: the file to load, relative or absolute path
	:param None|set loaded_files: a set containing all files that have already been loaded
	:rtype: unicode
	:return: the program including its includes
	"""
	filename = os.path.abspath(filename)
	root = os.path.dirname(filename)

	if loaded_files is None:
		loaded_files = set()
	loaded_files.add(filename)

	with open(filename, 'rb') as f:
		program = f.read().decode('utf-8')

	def include(match):
		included_file = os.path.abspath(os.path.join(root, match.group(1)))
		if included_file in loaded_files:
			return ''
		return load_file_with_includes(included_file, loaded_files)+'\n'

	return re.sub('#include\s+"(.*?)"\n', include, program)




def parse_lambda_code(s):
	"""
	Parses given lambda expressions and returns them as AST tree. Root is always a Program instance.
	:param unicode|str s:
	:rtype: Program
	:return:
	"""
	if isinstance(s, str) and not isinstance(s, unicode):
		s = s.decode('utf-8')
	return parse(s, Program).normalize()


if __name__ == '__main__':
	program = parse_lambda_code(TEST)
	print(program)
	print('\n')
	print(program.to_cpp())
