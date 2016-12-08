import ast
import lambdaparser
import types
import islpy as isl

ASSUME_CODE = '''
assume opNatPlus: (Nat, Nat) -> Nat;
assume opNatMinus: (Nat, Nat) -> Nat;
assume reduce: pi t:*. pi op: ((t, t) -> t). pi startval:t. pi len:Nat. pi values:(Nat -> t). t;
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


# ----- create Assume instances, and add types -----
assumes = dict(lambdaparser.parse_lambda_code(ASSUME_CODE).to_ast())
assumes['opNatPlus'].get_constraints = types.MethodType(nat_add_constraint, assumes['opNatPlus'])
assumes['opNatMinus'].get_constraints = types.MethodType(nat_sub_constraint, assumes['opNatMinus'])
assumes['reduce'].get_constraints = types.MethodType(reduce_constraints, assumes['reduce'])

for name, assumption in assumes.items():
	assumption.predefined = True
	ast.set_assumption(name, assumption)
