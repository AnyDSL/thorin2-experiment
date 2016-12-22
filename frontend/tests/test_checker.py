# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
from lambdaparser import parse_lambda_code
import ast



CODE1 = '''
define x1 = 1;
@guarantees "v1+v2 = v3"
assume opNatPlus: (Nat,Nat)->Nat;
@accepts "0 <= v1 < 10"
assume intakeLower10: Nat->Nat;
define x3 = opNatPlus (1,2);
define x4 = (lambda y: Nat. y) 1;
define x4 = lambda y: Nat. (y, y);
define x5 = (lambda y: Nat. (y, y)) 1;
define x6 = (lambda y: Nat. 1);

define c1 = intakeLower10 0;
define c2 = intakeLower10 12;
define c3 = intakeLower10 (opNatPlus (3, 5));

// Natural and floating-point numbers
assume Float: *;
assume cFloatZero: Float;

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
		define c = opNatPlus a b;
	in (n, a, b, c) end;
'''

CODE3 = '''
@accepts "0 <= v1 < 10"
assume intakeLower10: Nat->Nat;

//define if1 = lambda n: Nat. if_gez Nat n (1, 2);
define if2 = lambda n: Nat. if_gez Nat n (intakeLower10 n, intakeLower10 (opNatPlus (n,7)));
'''

CODE6 = '''
define t1 = lambda rec f (x:Nat):Nat. if_gz Nat x (x, f (opNatPlus(x, 5)));

@accepts "0 <= v1 < 10"
assume intakeLower10: Nat->Nat;
define t2 = lambda n:Nat. lambda rec f1(i:Nat):Nat. intakeLower10(i);
define t3 = lambda n:Nat. lambda rec f2(i:Nat):Nat. intakeLower10(n);
'''

CODE4 = '''
define test = lambda n:Nat. let
	define f1 = lambda rec f1_inner (xy:sigma(Nat,Nat)): Nat. f2(xy[1], 0);
	define f2 = lambda rec f2_inner (xy:sigma(Nat,Nat)): Nat. f1(0, xy[0]);
in f1(15, 3) end;
'''


class TestChecker(unittest.TestCase):
	def test_something(self):
		self.assertEqual(True, False)

	def test_matrix_multiplication(self):
		pass

	def test_lu_decomposition(self):
		pass


if __name__ == '__main__':
	unittest.main()
