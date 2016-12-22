# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import lambdaparser

'''
This is the main interface to the Lambda CoC frontend.
Functions:
- loading programs from file/string
- Converting to C++ representation
- Converting to frontend AST
- Constraint-checking
'''


def load_program(fname):
	code = lambdaparser.load_file_with_includes(fname)
	return lambdaparser.parse_lambda_code(lambdaparser.strip_comments(code))

def load_program_from_string(code):
	if isinstance(code, str):
		code = code.decode('utf-8')
	return lambdaparser.parse_lambda_code(lambdaparser.strip_comments(code))



def translate_file_to_cpp(filename, output_filename=None):
	program = load_program(filename)
	cpp_program = program.to_cpp()
	if output_filename is None:
		output_filename = filename+'.h'
	with open(output_filename, 'w') as f:
		f.write(cpp_program.encode('utf-8'))
	print('File "{}" written.'.format(output_filename))



def check_constraints_on_def(definition):
	#TODO
	pass

def check_constraints_on_program(program):
	#TODO
	pass



def main():
	# see this as an API / features demo
	for filename in ['matrixmultiplication.lbl', 'lu_decomposition.lbl']:
		program = load_program('programs/'+filename)
		definitions = program.to_ast()
		print filename, 'includes', len(definitions), 'definitions / assumptions'
		for name, definition in definitions:
			check_constraints_on_def(definition)
		# finally transform this program for the C++ CoC
		translate_file_to_cpp('programs/'+filename)


if __name__ == '__main__':
	main()
