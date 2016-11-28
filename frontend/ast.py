# -*- coding: utf-8 -*-

import re
import types
import islpy as isl

ctx = isl.DEFAULT_CONTEXT
last_var_number = 0
def get_var_name(name =''):
	global last_var_number
	last_var_number += 1
	return str(name)+'_'+str(last_var_number)


def set_join(a, b):
	dim_a = a.space.dim(isl.dim_type.set)
	dim_b = b.space.dim(isl.dim_type.set)
	vars  = [a.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim_a)]
	vars += [b.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim_b)]
	vars = list(set(vars))
	if dim_a + dim_b == len(vars):
		return a.flat_product(b)
	space = isl.Space.create_from_names(ctx, set=vars)
	result = isl.BasicSet.universe(space)
	for constraint in a.get_constraints():
		result.add_constraint(constraint)
	return result

def constraint_equal(bset, v1, *args):
	for v2 in args:
		if v2 != v1:
			bset = bset.add_constraint(isl.Constraint.eq_from_names(bset.space, {v1: 1, v2: -1}))
	return bset

def flatten(x):
	if isinstance(x, list):
		result = []
		for y in x:
			result += flatten(y)
		return result
	return [x]


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
		"""
		:return: (varname-list, accepted, possible)
		"""
		raise Exception('TODO '+repr(self.__class__))

	def check_constraints(self):
		vars, accepted, possible = self.get_constraints()
		# possible subsetof accepted => everything right
		if possible.is_subset(accepted):
			print '[VALID]'
			return True
		else:
			print '???'
			return None


	def get_nat_variables(self):
		vars = [op.get_nat_variables() for op in self.ops if isinstance(op, AstNode)]
		if len(vars) == 1:
			return vars[0]
		return vars


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

	def get_nat_variables(self):
		raise Exception('NI')


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

	def get_nat_variables(self):
		if self.name == 'Nat':
			return [get_var_name('p')]
		else:
			raise Exception('NI')


class ParamDef(AstNode):
	def __init__(self, name, type):
		if isinstance(type, Tupel):
			type = Sigma(type.ops)
		AstNode.__init__(self, [type])
		assert not isinstance(name, AstNode)
		self.name = str(name)
		self.constraint_vars = []

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

	def get_constraints(self):
		vars = self.constraint_vars
		unique_vars = set(flatten(vars))
		space = isl.Space.create_from_names(ctx, set=unique_vars)
		bset = isl.BasicSet.universe(space)
		return (vars, bset, bset)

	def get_nat_variables(self):
		raise Exception('NI')

	def create_new_constraint_vars(self):
		self.constraint_vars = self.ops[0].get_nat_variables()
		return self.constraint_vars


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

	def get_constraints(self):
		param_vars = self.ops[0].create_new_constraint_vars()
		body_vars, body_accepted, body_possible = self.ops[1].get_constraints()
		return ([param_vars, body_vars], body_accepted, body_possible)



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

	def get_constraints(self):
		func_vars, func_accepted, func_possible = self.ops[0].get_constraints()
		param_vars, param_accepted, param_possible = self.ops[1].get_constraints()
		accepted = set_join(func_accepted, param_accepted)
		possible = set_join(func_possible, param_possible)

		#TODO type error: forall[free vars]. exists[bound vars]. possible not subset of accepted
		#

		func_param_vars = flatten(func_vars[0])
		param_vars = flatten(param_vars)
		assert len(func_param_vars) == len(param_vars)
		for vf, vp in zip(func_param_vars, param_vars):
			accepted = constraint_equal(accepted, vf, vp)
			possible = constraint_equal(possible, vf, vp)
		return (func_vars[1], accepted, possible)



class Tupel(AstNode):
	def __str__(self):
		return '('+', '.join(map(str, self.ops))+')'

	def get_type(self):
		return Sigma([op.get_type() for op in self.ops])

	def get_constraints(self):
		vars, accepted, possible = self.ops[0].get_constraints()
		vars = [vars]
		for op in self.ops[1:]:
			vars2, accepted2, possible2 = op.get_constraints()
			vars.append(vars2)
			accepted = set_join(accepted, accepted2)
			possible = set_join(possible, possible2)
		return (vars, accepted, possible)




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
	vname = get_var_name('c' + str(name))
	space = isl.Space.create_from_names(ctx, set=[vname])
	accepted = isl.BasicSet.universe(space)
	possible = accepted.add_constraint(isl.Constraint.eq_from_names(space, {vname: 1, 1: -int(name)}))
	return ([vname], accepted, possible)


def nat_add_constraint(self):
	a = get_var_name()
	b = get_var_name()
	c = get_var_name()
	space = isl.Space.create_from_names(ctx, set=[a, b, c])
	accepted = isl.BasicSet.universe(space)
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {a: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {b: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {c: 1}))
	possible = accepted.add_constraint(isl.Constraint.eq_from_names(space, {a: 1, b: 1, c: -1}))
	return ([[a, b], c], accepted, possible)



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

