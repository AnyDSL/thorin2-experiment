import ast
import lambdaparser
import types


CODE = '''
define x1 = 1;
assume opAddNat: (Nat,Nat)->Nat;
define x3 = opAddNat (1,2);
define x4 = (lambda y: Nat. y) 1;
define x4 = lambda y: Nat. (y, y);
define x5 = (lambda y: Nat. (y, y)) 1;
'''

prog = lambdaparser.parse_lambda_code(CODE)
print prog
nodes = prog.to_ast()

# manual typing
ass = ast.get_assumption('opAddNat')
ass.get_constraints = types.MethodType(ast.nat_add_constraint, ass)

for root in nodes:
	print '- ',root,
	print ': ',root.get_type()
	print 'Constraints: ', root.get_constraints()




def test_typing():
	with open('arrays.lbl', 'r') as f:
		arraycode = f.read()
	prog = lambdaparser.parse_lambda_code(arraycode)
	nodes = prog.to_ast()
	for root in nodes:
		print '- ', root,
		print ': ', root.get_type()



#test_typing()
