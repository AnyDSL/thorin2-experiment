import ast
import lambdaparser
import types
import islpy as isl

ASSUME_CODE = '''
assume opNatPlus: (Nat, Nat) -> Nat;
assume opNatMinus: (Nat, Nat) -> Nat;
assume reduce: pi t:*. pi op: ((t, t) -> t). pi startval:t. pi len:Nat. pi values:(Nat -> t). t;

assume if_gez: pi t:*. Nat -> sigma(t, t) ->  t;
assume if_gz: pi t:*. Nat -> sigma(t, t) ->  t;
assume if_lez: pi t:*. Nat -> sigma(t, t) ->  t;
assume if_lz: pi t:*. Nat -> sigma(t, t) ->  t;
'''

# ----- pre-defined typing functions -----

def nat_add_constraint(self):
	vars, accepted, possible = self.get_isl_sets()
	a, b, c = ast.flatten(vars)
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {a: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {b: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {c: 1}))
	possible = accepted.add_constraint(isl.Constraint.eq_from_names(accepted.space, {a: 1, b: 1, c: -1}))
	return ([[a, b], [c]], accepted, possible)

def nat_sub_constraint(self):
	vars, accepted, possible = self.get_isl_sets()
	a, b, c = ast.flatten(vars)
	possible = accepted.add_constraint(isl.Constraint.eq_from_names(accepted.space, {a: 1, b: -1, c: -1}))
	return (vars, accepted, possible)

def reduce_constraints(self):
	var_len = ast.get_var_name('len')
	var_i = ast.get_var_name('i')
	vars = [[], [[[[], []], []], [[], [[var_len], [[[var_i], []], []]]]]]
	space = isl.Space.create_from_names(ast.ctx, set=[var_len, var_i])
	possible = isl.BasicSet.universe(space)
	accepted = isl.BasicSet.universe(space)
	# 0 <= i <= (len-1)
	possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_i: 1}))
	possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_len: 1, 1: -1, var_i: -1}))
	return (vars, accepted, possible)


class IfFunction(ast.SpecialFunction):
	def __init__(self, name, type, cond_eval_dict = {'cond': 1, 1: 0}):
		ast.SpecialFunction.__init__(self, name, type, 3)
		self.cond_eval_dict = cond_eval_dict

	def get_condition_constraint(self, space, cond_var):
		return isl.Constraint.ineq_from_names(space, {cond_var: self.cond_eval_dict['var'], 1: self.cond_eval_dict[1]})

	def get_app_constraints(self, app, params):
		# params = [type, cond, branches]
		assert isinstance(params[2], ast.Tupel)
		branch_true, branch_false = params[2].ops
		cond_vars, cond_accepted, cond_possible = params[1].get_constraints()
		cond_var, = ast.flatten(cond_vars)
		branch_true_vars, branch_true_accepted, branch_true_possible = branch_true.get_constraints()
		branch_false_vars, branch_false_accepted, branch_false_possible = branch_false.get_constraints()
		# create new vars, set == true/false variables
		result_vars = ast.clone_variables(branch_true_vars)
		all_vars = ast.flatten(result_vars) + ast.set_vars(branch_true_accepted) + ast.set_vars(branch_false_accepted)
		basic_accepted = ast.add_variables(cond_accepted, all_vars)
		basic_possible = ast.add_variables(cond_possible, all_vars)
		# accepted1: accepted_true && cond_true && cond_accept + vars from 2, without constraints
		# accepted = cond && result vars && branch accept
		branch_true_accepted = ast.set_join(basic_accepted, branch_true_accepted)
		branch_true_possible = ast.set_join(basic_possible, branch_true_possible)
		branch_false_accepted = ast.set_join(basic_accepted, branch_false_accepted)
		branch_false_possible = ast.set_join(basic_possible, branch_false_possible)
		# condition constraint (true/false branch taken)
		branch_true_constraint = self.get_condition_constraint(branch_true_accepted.space, cond_var)
		branch_false_constraint = ast.constraint_inverse(self.get_condition_constraint(branch_false_accepted.space, cond_var))
		# accepted &= cond_true / set result
		branch_true_accepted = branch_true_accepted.add_constraint(branch_true_constraint)
		#branch_true_accepted = ast.constraint_vars_equal(branch_true_accepted, result_vars, branch_true_vars)
		branch_false_accepted = branch_false_accepted.add_constraint(branch_false_constraint)
		#branch_false_accepted = ast.constraint_vars_equal(branch_false_accepted, result_vars, branch_false_vars)
		branch_true_possible = branch_true_possible.add_constraint(branch_true_constraint)
		branch_true_possible = ast.constraint_vars_equal(branch_true_possible, result_vars, branch_true_vars)
		branch_false_possible = branch_false_possible.add_constraint(branch_false_constraint)
		branch_false_possible = ast.constraint_vars_equal(branch_false_possible, result_vars, branch_false_vars)
		print 'IF result vars:',result_vars
		print '[TRUE]', branch_true_vars
		print '- A', branch_true_accepted
		print '- P', branch_true_possible
		print '[FALSE]', branch_false_vars
		print '- A', branch_false_accepted
		print '- P', branch_false_possible
		accepted = branch_true_accepted.union(branch_false_accepted)
		possible = branch_true_possible.union(branch_false_possible)
		print '=>', accepted, possible
		return (result_vars, accepted, possible)





# ----- create Assume instances, and add types -----
assumes = dict(lambdaparser.parse_lambda_code(ASSUME_CODE).to_ast())
assumes['opNatPlus'].get_constraints = types.MethodType(nat_add_constraint, assumes['opNatPlus'])
assumes['opNatMinus'].get_constraints = types.MethodType(nat_sub_constraint, assumes['opNatMinus'])
assumes['reduce'].get_constraints = types.MethodType(reduce_constraints, assumes['reduce'])
assumes['if_gez'] = IfFunction('if_gez', assumes['if_gez'].get_type(), {'var':  1, 1:  0})
assumes['if_gz']  = IfFunction('if_gz',  assumes['if_gz'].get_type(),  {'var':  1, 1: -1})
assumes['if_lez'] = IfFunction('if_lez', assumes['if_lez'].get_type(), {'var': -1, 1:  0})
assumes['if_lz']  = IfFunction('if_lz',  assumes['if_lz'].get_type(),  {'var': -1, 1: -1})

for name, assumption in assumes.items():
	assumption.predefined = True
	ast.set_assumption(name, assumption)
