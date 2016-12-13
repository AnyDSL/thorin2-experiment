# -*- coding: utf-8 -*-

import re
import types
import islpy as isl

ctx = isl.DEFAULT_CONTEXT
empty_bset = isl.BasicSet.universe(isl.Space.create_from_names(ctx, set=[]))

last_var_number = 0
def get_var_name(name =''):
	global last_var_number
	last_var_number += 1
	return str(name)+'_'+str(last_var_number)


def set_join(a, b):
	dim_a = a.space.dim(isl.dim_type.set)
	dim_b = b.space.dim(isl.dim_type.set)
	vars  = [a.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim_a)]
	vars2 = [b.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim_b)]
	for v in vars2:
		if v not in vars:
			vars.append(v)
	if dim_a + dim_b == len(vars):
		return a.flat_product(b)
	space = isl.Space.create_from_names(ctx, set=vars)
	result = isl.BasicSet.universe(space)
	for constraint in a.get_constraints():
		result = result.add_constraint(clone_constraint(result.space, constraint))
	for constraint in b.get_constraints():
		result = result.add_constraint(clone_constraint(result.space, constraint))
	return result

def clone_constraint(space, constraint):
	assert not constraint.is_div_constraint()
	c = {1: constraint.get_constant_val()}
	for i in xrange(constraint.space.dim(isl.dim_type.set)):
		c[constraint.get_dim_name(isl.dim_type.set, i)] = constraint.get_coefficient_val(isl.dim_type.set, i)
	if constraint.is_equality():
		return isl.Constraint.eq_from_names(space, c)
	else:
		return isl.Constraint.ineq_from_names(space, c)

def var_in_set(vname, bset):
	dim = bset.space.dim(isl.dim_type.set)
	return vname in (bset.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim))

def set_vars(bset):
	dim = bset.space.dim(isl.dim_type.set)
	return [bset.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim)]

def add_variables(bset, vars):
	vars2 = []
	for v in vars:
		if not var_in_set(v, bset) and not v in vars2:
			vars2.append(v)
	if len(vars) == 0:
		return bset
	return set_join(bset, isl.BasicSet.universe(isl.Space.create_from_names(ctx, set=vars)))


def constraint_equal(bset, v1, *args):
	for v2 in args:
		if v2 != v1:
			bset = bset.add_constraint(isl.Constraint.eq_from_names(bset.space, {v1: 1, v2: -1}))
	return bset

def constraint_vars_equal(bset, vars1, vars2):
	if isinstance(vars1, list):
		assert isinstance(vars2, list)
		assert len(vars1) == len(vars2)
		for v1, v2 in zip(vars1, vars2):
			bset = constraint_vars_equal(bset, v1, v2)
		return bset
	return constraint_equal(bset, vars1, vars2)

def constraint_inverse(constraint):
	assert not constraint.is_div_constraint()
	assert not constraint.is_equality()
	c = {1: - constraint.get_constant_val()}
	for i in xrange(constraint.space.dim(isl.dim_type.set)):
		c[constraint.get_dim_name(isl.dim_type.set, i)] = - constraint.get_coefficient_val(isl.dim_type.set, i)
	c[1] -= 1
	return isl.Constraint.ineq_from_names(constraint.space, c)

def flatten(x):
	if isinstance(x, list):
		result = []
		for y in x:
			result += flatten(y)
		return result
	return [x]

def simplify_set(s):
	return s.coalesce().detect_equalities().remove_redundancies()

def clone_variables(vars, translation = None):
	if isinstance(vars, list):
		return [clone_variables(x, translation) for x in vars]
	vname = get_var_name(vars)
	if translation is not None:
		translation[vars] = vname
	return vname

def check_variables_structure_equal(vars1, vars2, translation = None):
	if isinstance(vars1, list):
		if not isinstance(vars2, list):
			return False
		if len(vars1) != len(vars2):
			return False
		for v1, v2 in zip(vars1, vars2):
			if not check_variables_structure_equal(v1, v2, translation):
				return False
		return True
	if translation is not None:
		translation[vars1] = vars2
	return True


def valid_input_constraints(vars, accepted, possible):
	# find islset(vars) so that (forall other vars: possible subset of accepted)
	# => make (possible \ accepted) empty
	# => find islset(vars) so that not (exists other vars: possible \ accepted not empty)
	error_set = possible.subtract(accepted)
	possible2 = possible
	print '   vars', vars
	print '   accepted', accepted
	print '   possible', possible
	print '   error_set', error_set
	dim_err = error_set.space.dim(isl.dim_type.set)
	set_vars = [error_set.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim_err)]
	indices = [set_vars.index(v) for v in vars]
	for i in xrange(dim_err-1, -1, -1):
		if not i in indices:
			error_set = error_set.project_out(isl.dim_type.set, i, 1)
			possible2 = possible2.project_out(isl.dim_type.set, i, 1)
	return (error_set.complement().intersect(possible2), error_set)


def is_subset(left, right, equal_vars = {}):
	'''
	:param islpy.Set left:
	:param islpy.Set right:
	:return: (left subset of right, left-repr, right-repr)
	'''
	for k, v in list(equal_vars.items()):
		equal_vars[v] = k
	dim_left = left.space.dim(isl.dim_type.set)
	vars_left = [left.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim_left)]
	# variables in right, but not in left: make existential in right
	dim_right = right.space.dim(isl.dim_type.set)
	for i in xrange(dim_right-1, -1, -1):
		varname = right.space.get_dim_name(isl.dim_type.set, i)
		if varname not in equal_vars and varname not in vars_left:
			right = right.project_out(isl.dim_type.set, i, 1)
	# variables in left, but not in right: add to right
	dim_right = right.space.dim(isl.dim_type.set)
	vars_right = [right.space.get_dim_name(isl.dim_type.set, i) for i in xrange(dim_right)]
	for i in xrange(dim_left):
		varname = left.space.get_dim_name(isl.dim_type.set, i)
		if varname not in vars_right:
			if varname in equal_vars:
				varname = equal_vars[varname]
			if varname not in vars_right:
				right = right.add_dims(isl.dim_type.set, 1)
				right = right.set_dim_name(isl.dim_type.set, dim_right, varname)
				dim_right += 1
	# re-order (and rename) right
	assert dim_left == dim_right
	for i in xrange(dim_left):
		name_left = left.space.get_dim_name(isl.dim_type.set, i)
		name_right = right.space.get_dim_name(isl.dim_type.set, i)
		if name_left == name_right:
			continue
		elif name_left in equal_vars and equal_vars[name_left] == name_right:
			right = right.set_dim_name(isl.dim_type.set, i, name_left)
		else:
			right_index = right.find_dim_by_name(name_left)
			if right_index >= 0:
				right = right.move_dims(isl.dim_type.set, i, isl.dim_type.set, right_index, 1)
			else:
				right_index = right.find_dim_by_name(equal_vars[name_left])
				right = right.move_dims(isl.dim_type.set, i, isl.dim_type.set, right_index, 1)
				right = right.set_dim_name(isl.dim_type.set, i, name_left)
	print 'L',left
	print 'R',right
	# do the subset check
	return (left.is_subset(right), left, right)


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

	def check_constraints(self, constraints = None):
		if constraints:
			vars, accepted, possible = constraints
		else:
			vars, accepted, possible = self.get_constraints()
		# possible subsetof accepted => everything right
		if possible.is_subset(accepted):
			print '[VALID]'
			return True
		# possible can't be made accepted by other constraints
		if possible.is_disjoint(accepted):
			print '[INVALID]  (disjoint)'
			return False
		unbound_vars = flatten(vars)
		# any outer influence on the breaking parts?
		if len(unbound_vars) == 0:
			print '[INVALID]  (not satisfied, no influence on constraints remaining)'
			return False
		# constraints on "valid" inputs
		print '[???]'
		valid_set, error_set = valid_input_constraints(unbound_vars, accepted, possible)
		print 'Valid if:  ', simplify_set(valid_set)
		print 'Invalid if:', simplify_set(error_set)
		return None

	def get_nat_variables(self):
		vars = [op.get_nat_variables() for op in self.ops if isinstance(op, AstNode)]
		if len(vars) == 1:
			return vars[0]
		return vars

	def get_special_function_param_count(self):
		return 0







global_assumptions = {}

def get_assumption(name):
	if name in global_assumptions:
		return global_assumptions[name]
	na = create_assumption(name)
	if na:
		global_assumptions[name] = na
	return na

def set_assumption(name, node):
	global_assumptions[name] = node

def has_predefined_assumption(name):
	return name in global_assumptions and global_assumptions[name].predefined



class Assume(AstNode):
	# ops = [name, type]
	def __init__(self, ops):
		AstNode.__init__(self, ops)
		global_assumptions[ops[0]] = self
		self.predefined = False

	def __str__(self):
		return self.ops[0]

	def get_type(self):
		return self.ops[1]

	def get_name(self):
		return self.ops[0]

	def get_nat_variables(self):
		#print '[WARN] get_nat_variables not implemented for', self.ops[0]
		return self.ops[1].get_nat_variables()

	def get_constraints(self):
		vars, accepted, possible = self.get_isl_sets()
		print '[WARN] Untyped assume:', self.ops[0]
		print 'Typing as', (vars, accepted, possible)
		return (vars, accepted, possible)

	def get_isl_sets(self):
		vars = self.ops[1].get_nat_variables()
		bset = isl.BasicSet.universe(isl.Space.create_from_names(ctx, set=flatten(vars)))
		return (vars, bset, bset)


class SpecialFunction(Assume):
	# ops = [name, type]
	def __init__(self, name, type, param_count):
		Assume.__init__(self, [name, type])
		self.param_count = param_count

	def get_constraints(self):
		raise Exception('Special function '+str(self.ops[0])+' incorrectly used!')

	def get_app_constraints(self, app, params):
		raise Exception('Not implemented')

	def get_special_function_param_count(self):
		return self.param_count



class Constant(AstNode):
	def __init__(self, name):
		AstNode.__init__(self, [])
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
		elif self.name == '*':
			return []
		else:
			raise Exception('NI')

	def get_constraints(self):
		return ([], empty_bset, empty_bset)


class ParamDef(AstNode):
	#ops = [type]
	def __init__(self, name, type):
		if isinstance(type, Tupel):
			type = Sigma(type.ops)
		AstNode.__init__(self, [type])
		assert not isinstance(name, AstNode)
		self.name = str(name)
		self.constraint_vars = None
		self.default_vars = None

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
		if self.constraint_vars is not None:
			vars = self.constraint_vars
		else:
			vars = self.get_default_vars()
		unique_vars = set(flatten(vars))
		space = isl.Space.create_from_names(ctx, set=unique_vars)
		bset = isl.BasicSet.universe(space)
		return (vars, bset, bset)

	def get_nat_variables(self):
		return self.ops[0].get_nat_variables()

	def create_new_constraint_vars(self):
		self.constraint_vars = self.ops[0].get_nat_variables()
		return self.constraint_vars

	def get_default_vars(self):
		if self.default_vars is None:
			self.default_vars = self.get_nat_variables()
		return self.default_vars


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
		undefined_vars = [vname for vname in flatten(param_vars) if not var_in_set(vname, body_accepted)]
		if len(undefined_vars) > 0:
			space = isl.Space.create_from_names(ctx, set=undefined_vars)
			bset = isl.BasicSet.universe(space)
			body_accepted = set_join(body_accepted, bset)
			body_possible = set_join(body_possible, bset)
		return ([param_vars, body_vars], body_accepted, body_possible)


class LambdaNominal(Lambda):
	# ops = param, returntype, body||None
	def __init__(self, ops):
		if len(ops) == 2:
			ops.append(None)
		Lambda.__init__(self, ops)
		self.cstr_vars = None
		self.cstr_accepted = None
		self.cstr_possible = None
		self.outer_parameters = None

	def set_body(self, body):
		self.ops[2] = body

	def get_constraint_clone(self):
		translation = {}
		#TODO add outer params as identity
		vars = clone_variables(self.cstr_vars, translation)
		for i in xrange(self.cstr_accepted.space.dim(isl.dim_type.set)):
			v = self.cstr_accepted.space.get_dim_name(isl.dim_type.set, i)
			if not v in translation:
				translation[v] = get_var_name(v)
		accepted = self.cstr_accepted.copy()
		possible = self.cstr_possible.copy()
		for i in xrange(accepted.space.dim(isl.dim_type.set)):
			v_old = accepted.space.get_dim_name(isl.dim_type.set, i)
			if v_old in translation:
				accepted = accepted.set_dim_name(isl.dim_type.set, i, translation[v_old])
				possible = possible.set_dim_name(isl.dim_type.set, i, translation[v_old])
		return (vars, accepted, possible)

	def set_default_constraints(self):
		self.cstr_vars = self.get_nat_variables()
		bset = isl.BasicSet.universe(isl.Space.create_from_names(ctx, set=flatten(self.cstr_vars)))
		self.cstr_accepted = bset
		self.cstr_possible = bset

	def get_outer_parameters(self):
		if self.outer_parameters is not None:
			return self.outer_parameters
		self.outer_parameters = []
		def crawl(node):
			if isinstance(node, AstNode):
				if isinstance(node, LambdaNominal):
					self.outer_parameters.extend(node.get_outer_parameters())
				for op in self.ops:
					self.outer_parameters.extend(crawl(op))
		crawl(self.ops[2])

		
	def __str__(self):
		return 'λ rec ('+str(self.ops[0].name)+':'+str(self.ops[0].ops[0])+'): '+str(self.ops[1])

	def get_type(self):
		return Pi([self.ops[0], self.ops[1]])

	def subst(self, name, value):
		raise Exception('TODO')

	def get_constraints(self):
		if self.cstr_vars is not None:
			return self.get_constraint_clone()
		print '[WARN] Constraint checker did not inspect nominal lambda', self
		param_vars = self.ops[0].get_nat_variables()
		result_vars = self.ops[1].get_nat_variables()
		vars = [param_vars, result_vars]
		bset = isl.BasicSet.universe(isl.Space.create_from_names(ctx, set=flatten(vars)))
		return (vars, bset, bset)

	def get_nat_variables(self):
		return [self.ops[0].get_nat_variables(), self.ops[1].get_nat_variables()]

	def get_constraints_recursive(self):
		# copied from Lambda.get_constraints
		param_vars = self.ops[0].create_new_constraint_vars()
		body_vars, body_accepted, body_possible = self.ops[2].get_constraints()
		undefined_vars = [vname for vname in flatten(param_vars) if not var_in_set(vname, body_accepted)]
		if len(undefined_vars) > 0:
			space = isl.Space.create_from_names(ctx, set=undefined_vars)
			bset = isl.BasicSet.universe(space)
			body_accepted = set_join(body_accepted, bset)
			body_possible = set_join(body_possible, bset)
		return ([param_vars, body_vars], body_accepted, body_possible)






class Pi(AstNode):
	# ops = param, body
	def __str__(self):
		return 'Π'+str(self.ops[0].name)+':'+str(self.ops[0].ops[0])+'. '+str(self.ops[1])

	def get_type(self):
		raise Exception('TODO pi type')


class App(AstNode):
	def __init__(self, ops):
		AstNode.__init__(self, ops)
		self.special_param_count = ops[0].get_special_function_param_count()
		self.is_special_function_app = self.special_param_count > 0
		if self.special_param_count > 0:
			self.special_param_count -= 1

	def get_special_function_param_count(self):
		return self.special_param_count

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
		if self.is_special_function_app:
			return self.get_special_function_constraints()

		func_vars, func_accepted, func_possible = self.ops[0].get_constraints()
		param_vars, param_accepted, param_possible = self.ops[1].get_constraints()
		accepted = set_join(func_accepted, param_accepted)
		possible = set_join(func_possible, param_possible)

		func_param_vars = flatten(func_vars[0])
		param_vars_flat = flatten(param_vars)
		if len(func_param_vars) != len(param_vars_flat):
			print 'APP ERROR'
			print 'Function:', self.ops[0]
			print 'Param:   ', self.ops[1]
			print 'Func var type:', func_vars
			print 'Param type:   ', param_vars
			print 'Accepted:', func_param_vars
			print 'Given:   ', param_vars_flat
		assert len(func_param_vars) == len(param_vars_flat)
		for vf, vp in zip(func_param_vars, param_vars_flat):
			accepted = constraint_equal(accepted, vf, vp)
			possible = constraint_equal(possible, vf, vp)

		# TODO type error: forall[free vars]. exists[bound vars]. possible not subset of accepted
		#
		# Simple form for now
		if not possible.is_subset(accepted) and possible.is_disjoint(accepted):
			print '[ERR] This must go wrong: ', self

		return (func_vars[1], accepted, possible)

	def get_special_function_constraints(self):
		node = self
		params = []
		while isinstance(node, App):
			params = [node.ops[1]] + params
			node = node.ops[0]
		assert isinstance(node, SpecialFunction)
		return node.get_app_constraints(self, params)


	def get_nat_variables(self):
		func_vars = self.ops[0].get_nat_variables()
		#param_vars = self.ops[1].get_nat_variables()
		return func_vars[1]



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
	# ops = [tupel, index]
	def __init__(self, ops):
		AstNode.__init__(self, [ops[0], int(ops[1])])

	def __str__(self):
		return str(self.ops[0])+'['+str(self.ops[1])+']'

	def get_type(self):
		tupeltype = self.ops[0].get_type()
		assert isinstance(tupeltype, Sigma)
		return tupeltype.ops[self.ops[1]]

	def get_constraints(self):
		vars, accepted, possible = self.ops[0].get_constraints()
		vars = vars[self.ops[1]]
		return (vars, accepted, possible)

	def get_nat_variables(self):
		vars = self.ops[0].get_nat_variables()
		return vars[self.ops[1]]



def nat_const_constraints(self):
	name = self.ops[0]
	vname = get_var_name('c' + str(name))
	space = isl.Space.create_from_names(ctx, set=[vname])
	accepted = isl.BasicSet.universe(space)
	possible = accepted.add_constraint(isl.Constraint.eq_from_names(space, {vname: 1, 1: -int(name)}))
	return ([vname], accepted, possible)

def empty_constraints(self):
	return ([], empty_bset, empty_bset)

def auto_constraints(self):
	return self.get_isl_sets()



global_constants = {'Nat': Constant('Nat'), '*': Constant('*')}

def get_constant(name):
	return global_constants[name]


def create_assumption(name):
	# nat consts
	if re.match(r'^cNat\d+$', name):
		name = name[4:]
	if re.match(r'^\d+$', name):
		a = Assume([name, get_constant('Nat')])
		a.get_constraints = types.MethodType(nat_const_constraints, a)
		return a
	# float consts
	if re.match(r'^\d+\.\d+$', name):
		return Assume([name, get_assumption('Float')])
	return None


