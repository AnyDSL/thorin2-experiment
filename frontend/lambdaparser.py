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
assume 1: Nat;
assume opPlus: Nat -> Nat -> Nat;

// simple recursive
// define diverge = lambda (x:Nat): Nat. opPlus (diverge x) 1;
// internal recursive
define divergepoly = lambda t:*. lambda rec l1(x:t): t. (l1 x, x)[0];
define divergepoly2 = lambda t:*. lambda y:t. lambda rec l1(_:t): t. (l1 y, y)[0];

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

	def output(self, indent=''):
		return '\n'.join([indent + x for x in self.decls])



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

	def to_cpp(self, scope):
		# check if var in scope
		if self.name not in scope.vars:
			print('[WARN] Unknown variable:'+str(self.name))
			return str(self.name)
		# resolve var
		kind, vartype, acc = scope.vars[self.name]
		if kind == 'param':
			return 'w.var(' + vartype.to_cpp(scope) + ', ' + str(scope.depth - acc) + ', ' + cpp_string(self.name) + ')'
		return str(self.name)

	def __repr__(self):
		return 'Identifier('+repr(self.name)+')'


class Constant(Keyword, LambdaAST):
	grammar = Enum(K('*'), K('Nat'))

	def compose(self, parser, attr_of):
		return self.name

	def to_cpp(self, scope):
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

	def to_cpp(self, scope):
		if len(self) == 1:
			return self[0].to_cpp(scope)
		else:
			return '{'+', '.join([element.to_cpp(scope) for element in self])+'}'


class Lambda(Plain, LambdaAST):
	grammar = ['lambda', '\\', 'λ'], blank, [name(), '_'], ':', attr('type', Expression), '.', blank, attr('body', Expression)

	def compose(self, parser, attr_of):
		return 'λ ' + self.name + ':' + parser.compose(self.type, attr_of=self) + '. ' + parser.compose(self.body, attr_of=self)

	def to_cpp(self, scope):
		return 'w.lambda('+self.type.to_cpp(scope)+', '+self.body.to_cpp(scope.push_param(self.name, self.type))+')'


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

	def to_cpp(self, scope):
		#TODO
		return 'w.lambda('+self.type.to_cpp(scope)+', '+self.body.to_cpp(scope.push_param(self.name, self.type))+')'


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

	def to_cpp(self, scope):
		return 'w.pi('+self.type.to_cpp(scope)+', '+self.body.to_cpp(scope.push_param(self.name, self.type))+')'


class Extract(List, LambdaAST):
	grammar = attr('base', AtomicExpression), maybe_some('[', re.compile(r'\d+'), ']')

	"""def normalize(self):
		# remove Extract without indices
		if len(self) == 0:
			return self.base.normalize()
		else:
			return LambdaAST.normalize(self)"""

	def to_cpp(self, scope):
		result = self.base.to_cpp(scope)
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

	def to_cpp(self, scope):
		result = self.func.to_cpp(scope)
		for param in self:
			result = 'w.app('+result+', '+param.to_cpp(scope)+')'
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

	def to_cpp(self, scope):
		result = '// '+compose(self, autoblank=True)+'\n'
		body = self.body.to_cpp(scope)
		scope.add_definition(self.name)
		result += 'auto '+self.name+' = '+body+';\n'
		result += 'printValue('+self.name+');\n'
		if not self.type:
			result += 'printType(' + self.name + ');\n'
		return result + '\n'


class Assumption(Plain, LambdaAST):
	grammar = 'assume', blank, name(), ':', blank, attr("body", Expression), ';'

	def to_expr(self):
		return 'assume '+self.name+': '+self.body.to_expr()

	def to_cpp(self, scope):
		result = '// '+compose(self, autoblank=True)+'\n'
		body = self.body.to_cpp(scope)
		scope.add_definition(self.name, self.body)
		result += 'auto '+self.name+' = w.assume('+body+', '+cpp_string(self.name)+');\n'
		result += 'printType(' + self.name + ');\n\n'
		return result


class Comment(str, LambdaAST):
	grammar = [comment_cpp, comment_sh, comment_c]

	def to_cpp(self, scope):
		return ''


class Program(List, LambdaAST):
	grammar = -1, [Definition, Assumption, Comment]

	def to_cpp(self):
		scope = Scope()
		codegen = CppCodeGen()
		return ''.join(map(lambda x: x.to_cpp(scope, codegen), self))





def parse_lambda_code(s):
	return parse(s, Program).normalize()


if __name__ == '__main__':
	program = parse_lambda_code(TEST)
	print(program)
	print('\n')
	print(program.to_cpp())
	pass
