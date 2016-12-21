# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from unittest import TestCase
from lambdaparser import *


class TestLambdaParser(TestCase):
	def test_simple_define(self):
		program = parse_lambda_code('define star = *;')
		self.assertIsInstance(program, Program)
		self.assertEqual(len(program), 1)
		self.assertIsInstance(program[0], Definition)

	def test_redefine_nat(self):
		program = parse_lambda_code('define Nat = Nat;')
		self.assertEqual(len(program), 1)

	def test_simple_functions(self):
		program = parse_lambda_code('''
			define f1   = lambda x:Nat. x;
			define t1t  = pi x:Nat. Nat;
			define f1t2 = Nat -> Nat;
			define f2   = λ x:Nat. x;
			define t2t  = Π x:Nat. Nat;
		''')
		self.assertEqual(len(program), 5)
		self.assertIsInstance(program[0].body, Lambda)
		self.assertIsInstance(program[1].body, Pi)
		self.assertIsInstance(program[2].body, Pi)
		self.assertIsInstance(program[3].body, Lambda)
		self.assertIsInstance(program[4].body, Pi)

	def test_parameter_binding(self):
		program = parse_lambda_code('define f = lambda x:Nat. x;')
		self.assertEqual(program[0].body.name, 'x')
		program = parse_lambda_code('define f = lambda x:Nat. y;')
		self.assertEqual(program[0].body.name, 'x')
		program = parse_lambda_code('define f = lambda _:Nat. x;')
		self.assertEqual(program[0].body.name, '_')

	def test_tuple_sigma(self):
		program = parse_lambda_code('''
		define t1 = (x, x);
		define t2 = (x, x, x, x, x);
		define s1 = sigma(x,x);
		define s2 = tuple(x,x);
		''')
		self.assertIsInstance(program[0].body.func.base, Tupel)
		self.assertIsInstance(program[1].body.func.base, Tupel)
		self.assertIsInstance(program[2].body.func, SpecialApp)
		self.assertIsInstance(program[3].body.func, SpecialApp)

	def test_assume(self):
		program = parse_lambda_code('''
		assume a1: *;
		assume a2: Nat;
		assume a3: Nat -> Nat;
		assume a4: a1;
		''');
		self.assertIsInstance(program[0], Assumption)
		self.assertIsInstance(program[1], Assumption)
		self.assertIsInstance(program[2], Assumption)
		self.assertIsInstance(program[3], Assumption)

	def test_recursive_lambda(self):
		program = parse_lambda_code('''define f = lambda rec (x:Nat):Nat. f x;''')
		self.assertIsInstance(program[0].body, LambdaRec)

	def test_comments(self):
		parse_lambda_code('''
		// atajkwjld
		/* b */
		define a = *;
		// abc
		# abc
		''')

	ARRAY_DEFS = '''
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
	'''

	MATRIX_MULTIPLICATION = '''
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

	def test_array_definitions(self):
		program = parse_lambda_code(self.ARRAY_DEFS)
		self.assertEqual(len(program), 15)
		program = parse_lambda_code(self.MATRIX_MULTIPLICATION)
		self.assertEqual(len(program), 1)

	def test_lu_decomposition(self):
		with open('../lu-decomposition.lbl', 'rb') as f:
			program = f.read().decode('utf-8')
		program = parse_lambda_code(program)
		self.assertGreaterEqual(len(program), 26)


