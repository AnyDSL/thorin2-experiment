# -*- coding: utf-8 -*-

import re
import types
import islpy as isl

ctx = isl.DEFAULT_CONTEXT
last_var_number = 0
def getVarName(name = ''):
	global last_var_number
	last_var_number += 1
	return str(name)+'_'+str(last_var_number)


class AstNode:
	def __init__(self, ops):
		self.ops = ops
	def __repr__(self):
		return '{'+str(self)+'}'
	def __str__(self):
		return 'TODO'
	def __eq__(self, other):
		return self.__class__ == other.__class__ and self.ops == other.ops
	def __ne__(self, other):
		return not self == other

	def subst(self, name, value):
		ops = []
		for op in self.ops:
			if isinstance(op, AstNode):
				ops.append(op.subst(name, value))
			else:
				ops.append(op)
		return self.__class__(ops)

	def get_constraints(self):
		raise Exception('TODO '+repr(self.__class__))


global_assumptions = {}

def get_assumption(name):
	if name in global_assumptions:
		return global_assumptions[name]
	na = create_assumption(name)
	if na:
		global_assumptions[name] = na
	return na

class Assume(AstNode):
	# ops = [name, type]
	def __init__(self, ops):
		AstNode.__init__(self, ops)
		global_assumptions[ops[0]] = self

	def __str__(self):
		return self.ops[0]

	def get_type(self):
		return self.ops[1]


class Constant(AstNode):
	def __init__(self, name):
		self.name = name

	def __str__(self):
		return self.name

	def get_type(self):
		return get_constant('*')

	def __eq__(self, other):
		return other.__class__ == Constant and other.name == self.name

	def subst(self, name, value):
		return self


class ParamDef(AstNode):
	def __init__(self, name, type):
		if isinstance(type, Tupel):
			type = Sigma(type.ops)
		AstNode.__init__(self, [type])
		assert not isinstance(name, AstNode)
		self.name = str(name)

	def __str__(self):
		return self.name

	def get_type(self):
		return self.ops[0]

	def __eq__(self, other):
		return other.__class__ == self.__class__ and self.name == other.name and self.ops == other.ops

	def subst(self, name, value):
		if name == self.name:
			return value
		return ParamDef(self.name, self.ops[0].subst(name, value))


class Lambda(AstNode):
	# ops = param, body
	def __str__(self):
		return 'λ'+str(self.ops[0].name)+':'+str(self.ops[0].ops[0])+'. '+str(self.ops[1])

	def get_type(self):
		return Pi([self.ops[0], self.ops[1].get_type()])

	def subst(self, name, value):
		if name == self.ops[0].name:
			return self
		return AstNode.subst(self, name, value)


class Pi(AstNode):
	# ops = param, body
	def __str__(self):
		return 'Π'+str(self.ops[0].name)+':'+str(self.ops[0].ops[0])+'. '+str(self.ops[1])

	def get_type(self):
		raise Exception('TODO pi type')


class App(AstNode):
	def __str__(self):
		return str(self.ops[0])+'('+str(self.ops[1])+')'

	def get_type(self):
		func = self.ops[0].get_type()
		param = self.ops[1].get_type()
		assert isinstance(func, Pi)
		if func.ops[0].get_type() != param:
			#print 'Expected:',func.ops[0].get_type()
			#print 'Got:',param
			print '[WARN] Can\'t verify type integrity for appliation, use C++ CoC instead!'
		#assert func.ops[0].get_type() == param
		result = func.ops[1].subst(func.ops[0].name, self.ops[1])
		return result



class Tupel(AstNode):
	def __str__(self):
		return '('+', '.join(map(str, self.ops))+')'

	def get_type(self):
		return Sigma([op.get_type() for op in self.ops])


class Sigma(AstNode):
	def __str__(self):
		return 'Σ('+', '.join(map(str, self.ops))+')'

	def get_type(self):
		raise Exception('TODO sigma type')


class Extract(AstNode):
	def __init__(self, ops):
		AstNode.__init__(self, [ops[0], int(ops[1])])

	def __str__(self):
		return str(self.ops[0])+'['+str(self.ops[1])+']'

	def get_type(self):
		tupeltype = self.ops[0].get_type()
		assert isinstance(tupeltype, Sigma)
		return tupeltype.ops[self.ops[1]]




def nat_const_constraints(self):
	name = self.ops[0]
	vname = getVarName('c' + str(name))
	space = isl.Space.create_from_names(ctx, set=[vname])
	accepted = isl.BasicSet.universe(space)
	guaranteed = accepted.add_constraint(isl.Constraint.eq_from_names(space, {vname: 1, 1: -int(name)}))
	return ([vname], accepted, guaranteed)


def nat_add_constraint(self):
	a = getVarName()
	b = getVarName()
	c = getVarName()
	space = isl.Space.create_from_names(ctx, set=[a, b, c])
	accepted = isl.BasicSet.universe(space)
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {a: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {b: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {c: 1}))
	guaranteed = accepted.add_constraint(isl.Constraint.eq_from_names(space, {a: 1, b: 1, c: -1}))
	return ([a, b, c], accepted, guaranteed)



global_constants = {'Nat': Constant('Nat'), '*': Constant('*')}

def get_constant(name):
	return global_constants[name]


def create_assumption(name):
	# nat consts
	if re.match(r'^\d+$', name):
		a =  Assume([name, get_constant('Nat')])
		a.get_constraints = types.MethodType(nat_const_constraints, a)
		return a
	# float consts
	if re.match(r'^\d+\.\d+$', name):
		return Assume([name, get_assumption('Float')])
	return None

