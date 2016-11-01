#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import unicode_literals, print_function
from pypeg2 import *
import re

"""
REQUIREMENTS:
pip install pypeg2
"""



GRAMMAR = """
<definition> := define [type] <name> = <expression>;
<assumption> := assume <name>: <expression>;

<expression> :=   lambda <name>: <expression>. <expression>
				| pi <name>: <expression>. <expression>
				| <extract-expression> [<extract-expression>]+ [-> <expression>]
<extract-expression> := <atomic-expression> [ '[' <int> ']' ]+
<atomic-expression> := Nat | * | Param | Var
"""



TEST = """
define Nat = Nat;
assume n1: Nat;
assume opPlus: Nat -> Nat -> Nat;

// simple recursive
define diverge = lambda rec (x:Nat): Nat. opPlus (diverge x) n1;
// internal recursive
define divergepoly = lambda t:*. lambda rec l1(x:t): t. (l1 x, x)[0];
define divergepoly2 = lambda t:*. lambda y:t. lambda rec l2(_:t): t. (l2 y, y)[0];

// pairwise recursive
define d1 = lambda rec (x:Nat): Nat. d2 x;
define d2 = lambda rec (x:Nat): Nat. d3 x;
define d3 = lambda rec (x:Nat): Nat. d1 x;

"""



class Scope:
	def __init__(self):
		self.vars = {}
		self.depth = 0

	def push_param(self, vname=None, vtype=None):
		scope = Scope()
		scope.vars = self.vars.copy()
		if vname:
			scope.vars[vname] = ('param', vtype, self.depth)
		scope.depth = self.depth+1
		return scope

	def add_definition(self, defname, deftype=None):
		self.vars[defname] = ('def', deftype, 0)

	def contains(self, name):
		return name in self.vars


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



class LambdaAST:
	"""
	Removes all syntactic sugar from the AST.
	@:return the new representation of this node
	"""
	def normalize(self):
		if isinstance(self, List):
			for i in xrange(len(self)):
				if isinstance(self[i], LambdaAST):
					self[i] = self[i].normalize()
		for (k, v) in self.__dict__.items():
			if isinstance(v, LambdaAST):
				self.__dict__[k] = v.normalize()
		return self


def cpp_string(x):
	return '"'+x.replace('"', r'\"')+'"'





Expression = []
AtomicExpression = []
Keyword.regex = re.compile(r'\w+|\*')


class Identifier(str, LambdaAST):
	grammar = name()

	def to_cpp(self, scope, codegen):
		# check if var in scope
		if self.name not in scope.vars:
			#print('[WARN] Unknown variable:'+str(self.name))
			codegen.unknown_identifier(self.name)
			return str(self.name)
		# resolve var
		kind, vartype, acc = scope.vars[self.name]
		if kind == 'param':
			return 'w.var(' + vartype.to_cpp(scope, codegen) + ', ' + str(scope.depth - acc) + ', ' + cpp_string(self.name) + ')'
		return str(self.name)

	def __repr__(self):
		return 'Identifier('+repr(self.name)+')'


class Constant(Keyword, LambdaAST):
	grammar = Enum(K('*'), K('Nat'))

	def compose(self, parser, attr_of):
		return self.name

	def to_cpp(self, scope, codegen):
		if self.name == '*':
			return 'w.star()'
		elif self.name == 'Nat' and not scope.contains(self.name):
			return 'w.nat()'
		else:
			return self.name


class Tupel(List, LambdaAST):
	grammar = '(', csl(Expression), ')'

	def compose(self, parser, attr_of):
		return '('+', '.join([parser.compose(x, attr_of=self) for x in self])+')'

	def to_cpp(self, scope, codegen):
		if len(self) == 1:
			return self[0].to_cpp(scope, codegen)
		else:
			return '{'+', '.join([element.to_cpp(scope, codegen) for element in self])+'}'


class Lambda(Plain, LambdaAST):
	grammar = ['lambda', '\\', 'λ'], blank, [name(), '_'], ':', attr('type', Expression), '.', blank, attr('body', Expression)

	def compose(self, parser, attr_of):
		return 'λ ' + self.name + ':' + parser.compose(self.type, attr_of=self) + '. ' + parser.compose(self.body, attr_of=self)

	def to_cpp(self, scope, codegen):
		return 'w.lambda('+self.type.to_cpp(scope, codegen)+', '+self.body.to_cpp(scope.push_param(self.name, self.type), codegen)+')'


class LambdaRec(Plain, LambdaAST):
	#lambda rec [name](param:type): returntype. expression
	grammar = ['lambda', '\\', 'λ'], blank, K('rec'), blank, optional(name()), \
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

	def to_cpp(self, scope, codegen):
		# declare variable without body
		paramtype = self.type.to_cpp(scope, codegen);
		returntype = self.returntype.to_cpp(scope, codegen);
		varname = self.name or codegen.get_temp_varname()
		decl = 'auto ' + varname + ' = w.lambdaRec(' + paramtype + ', ' + returntype;
		if self.name:
			decl += ', '+cpp_string(self.name)
		codegen.add_decl(varname, decl + ');')

		# body decl
		scope.add_definition(varname, Pi.new('_', self.type, self.returntype))
		childscope = scope.push_param(self.param.thing, self.type)
		old_unknown = codegen.get_unknown_identifiers()
		codegen.set_unknown_identifiers_list([])
		body = self.body.to_cpp(childscope, codegen)
		codegen.add_depending('{}->setBody({});'.format(varname, body), codegen.get_unknown_identifiers())
		codegen.set_unknown_identifiers_list(old_unknown)
		return varname


class Pi(Plain, LambdaAST):
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

	def to_cpp(self, scope, codegen):
		return 'w.pi('+self.type.to_cpp(scope, codegen)+', '+self.body.to_cpp(scope.push_param(self.name, self.type), codegen)+')'


class Extract(List, LambdaAST):
	grammar = attr('base', AtomicExpression), maybe_some('[', re.compile(r'\d+'), ']')

	"""def normalize(self):
		# remove Extract without indices
		if len(self) == 0:
			return self.base.normalize()
		else:
			return LambdaAST.normalize(self)"""

	def to_cpp(self, scope, codegen):
		result = self.base.to_cpp(scope, codegen)
		for index in self:
			result = 'w.extract('+result+', '+index+')'
		return result


class App(List, LambdaAST):
	grammar = attr('func', Extract), maybe_some(Extract), optional('->', attr('arrow', Expression))

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
			return LambdaAST.normalize(self)

	def to_expr(self):
		return self.func.to_expr() + ' '.join([x.to_expr() for x in self])

	def compose(self, parser, attr_of):
		result = parser.compose(self.func, attr_of=self)
		for param in self:
			result += ' ' + parser.compose(param, attr_of=self)
		return result

	def to_cpp(self, scope, codegen):
		result = self.func.to_cpp(scope, codegen)
		for param in self:
			result = 'w.app('+result+', '+param.to_cpp(scope, codegen)+')'
		return result

	def __repr__(self):
		result = '('+repr(self.func);
		for param in self:
			result += '@'+repr(param)
		return result + ')'


Expression.extend([LambdaRec, Lambda, Pi, App])
AtomicExpression.extend([Tupel, Constant, Identifier])


class Definition(Plain, LambdaAST):
	grammar = 'define', blank, flag('type', 'type'), name(), blank, '=', blank, attr("body", Expression), ';'

	def compose(self, parser, attr_of):
		return self.name+' := '+parser.compose(self.body)

	def to_cpp(self, scope, codegen):
		codegen.add_comment('// '+compose(self, autoblank=True))
		body = self.body.to_cpp(scope, codegen)
		scope.add_definition(self.name)
		codegen.add_decl(self.name, 'auto '+self.name+' = '+body+';')
		codegen.add_inst('printValue('+self.name+');')
		if not self.type:
			codegen.add_inst('printType(' + self.name + ');')
		codegen.progress_depending()
		codegen.add_blank()


class Assumption(Plain, LambdaAST):
	grammar = 'assume', blank, name(), ':', blank, attr("body", Expression), ';'

	def to_expr(self):
		return 'assume '+self.name+': '+self.body.to_expr()

	def to_cpp(self, scope, codegen):
		codegen.add_comment('// '+compose(self, autoblank=True));
		body = self.body.to_cpp(scope, codegen)
		scope.add_definition(self.name, self.body)
		codegen.add_decl(self.name, 'auto '+self.name+' = w.assume('+body+', '+cpp_string(self.name)+');')
		codegen.add_inst('printType(' + self.name + ');')
		codegen.progress_depending()
		codegen.add_blank()


class Comment(str, LambdaAST):
	grammar = [comment_cpp, comment_sh, comment_c]

	def to_cpp(self, scope, codegen):
		return ''


class Program(List, LambdaAST):
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





def parse_lambda_code(s):
	return parse(s, Program).normalize()


if __name__ == '__main__':
	program = parse_lambda_code(TEST)
	print(program)
	print('\n')
	print(program.to_cpp())
	pass
