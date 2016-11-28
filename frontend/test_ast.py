import ast
import lambdaparser
import types
import islpy as isl


CODE = '''
define x1 = 1;
assume opAddNat: (Nat,Nat)->Nat;
assume intakeLower10: Nat->Nat;
define x3 = opAddNat (1,2);
define x4 = (lambda y: Nat. y) 1;
define x4 = lambda y: Nat. (y, y);
define x5 = (lambda y: Nat. (y, y)) 1;

define c1 = intakeLower10 0;
define c2 = intakeLower10 12;
define c3 = intakeLower10 (opAddNat (3, 5));
'''

prog = lambdaparser.parse_lambda_code(CODE)
print prog
nodes = prog.to_ast()

# manual typing
ass = ast.get_assumption('opAddNat')
ass.get_constraints = types.MethodType(ast.nat_add_constraint, ass)

def intakeLower10_constraints(self):
	a = ast.get_var_name()
	space = isl.Space.create_from_names(ast.ctx, set=[a])
	possible = isl.BasicSet.universe(space)
	accepted = isl.BasicSet.universe(space)
	accepted = accepted.add_constraint(isl.Constraint.ineq_from_names(accepted.space, {1: 10, a: -1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {a: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {b: 1}))
	# bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {c: 1}))
	#possible = accepted.add_constraint(isl.Constraint.eq_from_names(space, {a: 1, b: 1, c: -1}))
	return ([a, a], accepted, possible)

ass = ast.get_assumption('intakeLower10')
ass.get_constraints = types.MethodType(intakeLower10_constraints, ass)

for root in nodes:
	print '- ',root,
	print ': ',root.get_type()
	print 'Constraints: ', root.get_constraints()
	print root.check_constraints()
	print ''




def test_typing():
	with open('arrays.lbl', 'r') as f:
		arraycode = f.read()
	prog = lambdaparser.parse_lambda_code(arraycode)
	nodes = prog.to_ast()
	for root in nodes:
		print '- ', root,
		print ': ', root.get_type()



#test_typing()
