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

