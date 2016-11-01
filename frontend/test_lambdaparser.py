from __future__ import unicode_literals, print_function
import sys
import traceback
import lambdaparser



def test_floatvalues():
	program = lambdaparser.parse_lambda_code('assume Float: *; assume cFloatZero: Float;')
	program.to_cpp()


def test_identity():
	program = """
	define id_nat = lambda x: Nat. x;
	define id_poly = lambda t: *. lambda x: t. x;"""
	program = lambdaparser.parse_lambda_code(program)
	program.to_cpp()


def test_recursive():
	program = """
	define Nat = Nat;
	assume n1: Nat;
	assume opPlus: Nat -> Nat -> Nat;

	// simple recursive
	define diverge = lambda rec (x:Nat): Nat. opPlus (diverge x) n1;
	// internal recursive
	define divergepoly = lambda t:*. lambda rec l1(x:t): t. (l1 x, x)[0];
	define divergepoly2 = lambda t:*. lambda y:t. lambda rec l2(_:t): t. (l2 y, y)[0];

	// pairwise recursive
	define d1 = lambda rec (x:Nat): Nat. d2 x;
	define d2 = lambda rec (x:Nat): Nat. d3 x;
	define d3 = lambda rec (x:Nat): Nat. d1 x;
	"""







def run_all():
	for (k, v) in sys.modules[__name__].__dict__.items():
		if k.startswith('test_') and callable(v):
			print('[TEST] {} ...  '.format(k))
			try:
				v()
				print('[OK]   {} .'.format(k))
			except Exception, e:
				print(traceback.format_exc())




if __name__ == '__main__':
	run_all()

