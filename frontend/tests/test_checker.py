# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
from lambdaparser import parse_lambda_code, strip_comments
import ast
import checker
import frontend



CODE1 = '''
define x1 = 1;
@guarantees "v1+v2 = v3"
assume opNatPlus: (Nat,Nat)->Nat;
@accepts "0 <= v1 < 10"
assume intakeLower10: Nat->Nat;
define x2 = opNatPlus (1,2);
define x3 = (lambda y: Nat. y) 1;
define x4 = lambda y: Nat. (y, y);
define x5 = (lambda y: Nat. (y, y)) 1;
define x6 = (lambda y: Nat. 1);

define c1 = intakeLower10 0;
define c2 = intakeLower10 12;
define c3 = intakeLower10 (opNatPlus (3, 5));

// Natural and floating-point numbers
assume Float: *;
assume cFloatZero: Float;
assume opFloatPlus:  (Float, Float) -> Float;

// Array Type
@accepts "v1 >= 0"            // length must be >= 0
@guarantees "0 <= v2 < v1"    // type function is only called with parameter in [0, len[
assume ArrT: pi_:(Nat, Nat -> *). *;

// Array constructor (from callback lambda)
@accepts "type1 >= 0"
@guarantees "0 <= i < type1"
assume ArrCreate: pi type: (Nat, Nat -> *). (pi i: Nat. type[1] i) -> ArrT type;

// Array getter
@accepts "0 <= i < type1"
assume ArrGet: pi type: (Nat, Nat -> *). ArrT type -> pi i:Nat. type[1] i;

// Uniform arrays
define UArrT = lambda lt: (Nat, *). ArrT (lt[0], lambda _:Nat. lt[1]);

@guarantees "0 <= values1 < len"
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

CODE2 = '''
@guarantees "v1+v2 = v3"
assume opNatPlus: (Nat,Nat)->Nat;

define test1 = lambda n:Nat. let
		define a = n;
		define b = a;
		define c = opNatPlus (a, b);
	in (n, a, b, c) end;

define test2 = lambda rec (n:Nat): sigma(Nat, Nat, Nat, Nat). let
		define a = n;
		define b = a;
		define c = opNatPlus (a, b);
	in (n, a, b, c) end;
'''

CODE3 = '''
@guarantees "v1+v2 = v3"
assume opNatPlus: (Nat,Nat)->Nat;
@accepts "0 <= v1 < 10"
assume intakeLower10: Nat->Nat;

define if1 = lambda n: Nat. if_gez Nat n (1, 2);
define if2 = lambda n: Nat. if_gez Nat n (intakeLower10 n, intakeLower10 (opNatPlus (n,7)));
'''

CODE4 = '''
@guarantees "v1+v2 = v3"
assume opNatPlus: (Nat,Nat)->Nat;
@accepts "0 <= v1 < 10"
assume intakeLower10: Nat->Nat;

define t1 = lambda rec f (x:Nat):Nat. if_gz Nat x (x, f (opNatPlus(x, 5)));

define t2 = lambda n:Nat. lambda rec f1(i:Nat):Nat. intakeLower10(i);
define t3 = lambda n:Nat. lambda rec f2(i:Nat):Nat. intakeLower10(n);
'''

CODE5 = '''
define test = lambda n:Nat. let
	define f1 = lambda rec f1_inner (xy:sigma(Nat,Nat)): Nat. f2(xy[1], 0);
	define f2 = lambda rec f2_inner (xy:sigma(Nat,Nat)): Nat. f1(0, xy[0]);
in f1(15, 3) end;
'''





def get_var_mapping(vars, vars2):
	vars = ast.flatten(vars)
	vars2 = ast.flatten(vars2)
	assert len(vars) == len(vars2)
	result = {}
	for x, y in zip(vars, vars2):
		result[x] = y
	return result


class TestChecker(unittest.TestCase):
	from isl_test_utils import assert_isl_sets_equal

	def test_code_1(self):
		constraints = frontend.check_constraints_on_program(parse_lambda_code(strip_comments(CODE1)))

		self.assert_isl_sets_equal(constraints['x2'].possible, '{[c] : c = 3}', get_var_mapping(constraints['x2'].vars, ['c']), allow_set1_additional_vars=True)
		self.assert_isl_sets_equal(constraints['x3'].possible, '{[c] : c = 1}', get_var_mapping(constraints['x3'].vars, ['c']), allow_set1_additional_vars=True)
		self.assert_isl_sets_equal(constraints['x1'].possible, '{[c] : c = 1}', get_var_mapping(constraints['x1'].vars, ['c']), allow_set1_additional_vars=True)
		x4vars = ast.flatten(constraints['x4'].vars)
		self.assertEqual(len(x4vars), 3)
		self.assertEqual(len(set(x4vars)), 1)
		self.assert_isl_sets_equal(constraints['x6'].possible, '{[p, c] : c = 1}', get_var_mapping(constraints['x6'].vars, ['p', 'c']), allow_set1_additional_vars=True)

		self.assertTrue(constraints['c1'].is_valid())
		self.assertTrue(constraints['c2'].is_invalid())
		self.assertTrue(constraints['c3'].is_valid())

		self.assertTrue(constraints['a1'].is_valid())
		self.assertTrue(constraints['a2'].is_invalid())
		self.assertTrue(constraints['a3'].is_partially_valid())
		self.assert_isl_sets_equal(constraints['a3'].error_set, '{[n] : n <= 16}', get_var_mapping(constraints['a3'].vars, ['n']))
		self.assert_isl_sets_equal(constraints['a3'].valid_set, '{[n] : n > 16}', get_var_mapping(constraints['a3'].vars, ['n']))
		self.assertTrue(constraints['a4'].is_valid())

		self.assertTrue(constraints['rt1'].is_valid())
		self.assertTrue(constraints['rt2'].is_valid())
		constraints['rt3'].print_report()
		self.assertTrue(constraints['rt3'].is_invalid())
		self.assertTrue(constraints['rt4'].is_valid())

	def test_code_2(self):
		constraints = frontend.check_constraints_on_program(parse_lambda_code(strip_comments(CODE2)))
		self.assertTrue(constraints['test1'].is_valid())
		self.assertTrue(constraints['test2'].is_valid())
		self.assert_isl_sets_equal(constraints['test1'].possible, '{[a,b] : 2*a=b}',get_var_mapping(constraints['test1'].vars, ['a', 'a', 'a', 'a', 'b']), allow_set1_additional_vars=True)
		self.assert_isl_sets_equal(constraints['test2'].possible, '{[a,b] : 2*a=b}',get_var_mapping(constraints['test2'].vars, ['a', 'a', 'a', 'a', 'b']), allow_set1_additional_vars=True)

	def test_code_3(self):
		constraints = frontend.check_constraints_on_program(parse_lambda_code(strip_comments(CODE3)))
		# define if1 = lambda n: Nat. if_gez Nat n (1, 2);
		self.assertTrue(constraints['if1'].is_valid())
		self.assert_isl_sets_equal(constraints['if1'].possible, '{[a,b] : (a >= 0 and b = 1) or (a < 0 and b = 2) }',
									get_var_mapping(constraints['if1'].vars, ['a', 'b']), allow_set1_additional_vars=True)
		# define if2 = lambda n: Nat. if_gez Nat n (intakeLower10 n, intakeLower10 (opNatPlus (n,7)));
		self.assert_isl_sets_equal(constraints['if2'].valid_set, '{[a,b] : (0 <= a < 10) or (a < 0 and 0 <= a+7 < 10) }',
									get_var_mapping(constraints['if2'].vars, ['a', 'b']),
									allow_set1_additional_vars=True)

	def test_code_4(self):
		constraints = frontend.check_constraints_on_program(parse_lambda_code(strip_comments(CODE4)))
		# define t1 = lambda rec f (x:Nat):Nat. if_gz Nat x (x, f (opNatPlus(x, 5)));
		self.assertTrue(constraints['t1'].is_valid())
		# define t2 = lambda n:Nat. lambda rec f1(i:Nat):Nat. intakeLower10(i);
		self.assertTrue(constraints['t2'].is_partially_valid())
		self.assert_isl_sets_equal(constraints['t2'].valid_set, '{[i,n] : 0 <= i < 10 }',
									get_var_mapping(constraints['t2'].vars, ['n', 'i', 'r']),
									allow_set1_additional_vars=True)
		# define t3 = lambda n:Nat. lambda rec f2(i:Nat):Nat. intakeLower10(n);
		self.assertTrue(constraints['t3'].is_partially_valid())
		self.assert_isl_sets_equal(constraints['t3'].valid_set, '{[i,n] : 0 <= n < 10 }',
									get_var_mapping(constraints['t3'].vars, ['n', 'i', 'r']),
									allow_set1_additional_vars=True)

	def test_code_5(self):
		constraints = frontend.check_constraints_on_program(parse_lambda_code(strip_comments(CODE5)))
		self.assertTrue(constraints['test'].is_valid())

	def test_matrix_multiplication(self):
		program = frontend.load_program('../programs/matrixmultiplication.lbl')
		constraints = frontend.check_constraints_on_program(program)
		self.assertTrue(constraints['matrixDot'].is_valid())

	def test_lu_decomposition(self):
		program = frontend.load_program('../programs/lu_decomposition.lbl')
		constraints = frontend.check_constraints_on_program(program)
		self.assertTrue(constraints['LUD'].is_valid())
		self.assertTrue(constraints['LUD_without_constraints'].is_valid())


if __name__ == '__main__':
	unittest.main()
