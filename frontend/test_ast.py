import ast
import lambdaparser
import types
import islpy as isl
import predefined_functions
import checker


CODE = '''
define x1 = 1;
assume opNatPlus: (Nat,Nat)->Nat;
assume intakeLower10: Nat->Nat;
define x3 = opNatPlus (1,2);
define x4 = (lambda y: Nat. y) 1;
define x4 = lambda y: Nat. (y, y);
define x5 = (lambda y: Nat. (y, y)) 1;
define x6 = (lambda y: Nat. 1);

define c1 = intakeLower10 0;
define c2 = intakeLower10 12;
define c3 = intakeLower10 (opNatPlus (3, 5));
'''

CODE2 = '''
// Natural and floating-point numbers
define Nat = Nat;
assume Float: *;
assume opFloatPlus: (Float, Float) -> Float;
assume opFloatMult: (Float, Float) -> Float;
assume cFloatZero: Float;

// Arrays
assume ArrT: pi_:(Nat, Nat -> *). *;
assume ArrCreate: pi type: (Nat, Nat -> *). (pi i: Nat. type[1] i) -> ArrT type;
assume ArrGet: pi type: (Nat, Nat -> *). ArrT type -> pi i:Nat. type[1] i;
define UArrT = lambda lt: (Nat, *). ArrT (lt[0], lambda _:Nat. lt[1]);

//define MatrixType = lambda n:Nat. lambda m:Nat. ArrT (n, (lambda _:Nat. ArrT (m, (lambda _:Nat. Float))));
define MatrixType = lambda n:Nat. lambda m:Nat. UArrT (n, UArrT (m, Float));

assume reduce: pi t:*. pi op: ((t, t) -> t). pi startval:t. pi len:Nat. pi values:(Nat -> t). t;
define sum = reduce Float opFloatPlus cFloatZero;

// check this:
assume testarray1: ArrT (15, lambda _:Nat. Float);
define a1 = ArrGet (15, lambda _:Nat. Float) testarray1 0;
define a2 = ArrGet (15, lambda _:Nat. Float) testarray1 16;
define a3 = lambda n:Nat. ArrGet (n, lambda _:Nat. Float) testarray1 16;
define a4 = (lambda n:Nat. ArrGet (n, lambda _:Nat. Float) testarray1 16) 20;

define rt1 = sum 10 (lambda i:Nat. (intakeLower10 i, cFloatZero)[1]);
define rt2 = sum 15 (ArrGet (15, lambda _:Nat. Float) testarray1);
define rt3 = sum 20 (ArrGet (15, lambda _:Nat. Float) testarray1);
define rt4 = sum 0 (ArrGet (15, lambda _:Nat. Float) testarray1);
'''

CODE3 = '''
define matrixDot = lambda n:Nat. lambda m:Nat. lambda o:Nat. lambda M1: MatrixType n m. lambda M2: MatrixType m o.
		ArrCreate (n, lambda _:Nat. UArrT (o, Float)) (
			lambda i:Nat. ArrCreate (o, lambda _:Nat. Float) (lambda j: Nat.
				sum m (lambda k: Nat. opFloatMult(
					(ArrGet (m, lambda _:Nat. Float) (ArrGet (n, lambda _:Nat. UArrT (m, Float)) M1 i) k),
					(ArrGet (o, lambda _:Nat. Float) (ArrGet (m, lambda _:Nat. UArrT (m, Float)) M1 k) j)
				))
			)
		);
'''

CODE4 = '''
assume opNatPlus: (Nat,Nat)->Nat;

define test1 = lambda n:Nat. let
		define a = n;
		define b = a;
		define c = opNatPlus (a, b);
	in (n, a, b, c) end;

define test2 = lambda rec (n:Nat): sigma(Nat, Nat, Nat, Nat). let
		define a = n;
		define b = a;
		define c = opNatPlus a b;
	in (n, a, b, c) end;
'''

CODE5 = '''
assume intakeLower10: Nat->Nat;

//define if1 = lambda n: Nat. if_gez Nat n (1, 2);
define if2 = lambda n: Nat. if_gez Nat n (intakeLower10 n, intakeLower10 (opNatPlus (n,7)));
'''

CODE6 = '''
define t1 = lambda rec f (x:Nat):Nat. if_gz Nat x (x, f (opNatPlus(x, 5)));

assume intakeLower10: Nat->Nat;
define t2 = lambda n:Nat. lambda rec (i:Nat):Nat. intakeLower10(n);
'''


def type_manually():
	# manual typing
	def intakeLower10_constraints(self):
		a = ast.get_var_name()
		space = isl.Space.create_from_names(ast.ctx, set=[a])
		possible = isl.BasicSet.universe(space)
		accepted = isl.BasicSet.universe(space)
		accepted = accepted.add_constraint(isl.Constraint.ineq_from_names(accepted.space, {1: 10, a: -1})) # 0 <= 1*10 + -1*a
		accepted = accepted.add_constraint(isl.Constraint.ineq_from_names(accepted.space, {a: 1})) # 0 <= a
		return ([[a], [a]], accepted, possible)

	ass = ast.get_assumption('intakeLower10')
	if ass:
		ass.get_constraints = types.MethodType(intakeLower10_constraints, ass)

	def arr_get_constraints(self):
		var_len = ast.get_var_name('len')
		var_type_index = ast.get_var_name('ti')
		var_index = ast.get_var_name('i')
		vars = [[[var_len], [[var_type_index], []]], [[], [[var_index], []]]]
		space = isl.Space.create_from_names(ast.ctx, set=[var_len, var_type_index, var_index])
		possible = isl.BasicSet.universe(space)
		accepted = isl.BasicSet.universe(space)
		# accept only 0 <= i <= (len-1)
		#accepted = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_index: 1}))
		accepted = accepted.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_len: 1, 1: -1, var_index: -1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_type_index: 1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {var_len: 1, 1: -1, var_type_index: -1}))
		return (vars, accepted, possible)

	ass = ast.get_assumption('ArrGet')
	if ass:
		ass.get_constraints = types.MethodType(arr_get_constraints, ass)

	def arr_create_constraints(self):
		vars, accepted, possible = self.get_isl_sets()
		v_len, v_typeindex, v_valueindex = ast.flatten(vars)
		# 0 <= (value|type)index < len
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {v_len: 1, 1: -1, v_valueindex: -1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {v_valueindex: 1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {v_len: 1, 1: -1, v_typeindex: -1}))
		possible = possible.add_constraint(isl.Constraint.ineq_from_names(possible.space, {v_typeindex: 1}))
		return vars, accepted, possible

	ass = ast.get_assumption('ArrCreate')
	if ass:
		ass.get_constraints = types.MethodType(arr_create_constraints, ass)

	# type unnecessary stuff
	for x in ['Float', 'cFloatZero']:
		ass = ast.get_assumption(x)
		if ass:
			ass.get_constraints = types.MethodType(ast.empty_constraints, ass)

	for x in ['opFloatPlus', 'opFloatMinus', 'opFloatMult', 'opFloatDiv']:
		ass = ast.get_assumption(x)
		if ass:
			ass.get_constraints = types.MethodType(ast.auto_constraints, ass)





# add array stuff
#with open('arrays.lbl', 'r') as f:
#	arraycode = f.read()
#	arraycode = '\n'.join(arraycode.split('\n')[:18])
#	CODE += arraycode

CODE += CODE2
CODE += CODE3
#CODE = CODE4
#CODE = CODE5
CODE = CODE6
prog = lambdaparser.parse_lambda_code(CODE)
print prog
nodes = prog.to_ast()

type_manually()

for name, root in nodes:
	print '- ',name, ':=', root,
	print ': ',root.get_type()
	#constraints = root.get_constraints()
	#print 'Constraints: ', constraints
	#root.check_constraints(constraints)
	checker.check_definition(root)
	print ''




def test_typing():
	with open('arrays.lbl', 'r') as f:
		arraycode = f.read()
	prog = lambdaparser.parse_lambda_code(arraycode)
	nodes = prog.to_ast()[:-1] # skip matrix dot for now
	type_manually()

	for name, root in nodes:
		print '- ', name, ':=', root,
		print ': ', root.get_type()
		constraints = root.get_constraints()
		print 'Constraints: ', constraints
		print root.check_constraints()
		print ''
#test_typing()
