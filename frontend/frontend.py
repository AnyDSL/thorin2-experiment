# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import lambdaparser
import checker
import ast
from collections import OrderedDict
import sys

'''
This is the main interface to the Lambda CoC frontend.
Functions:
- loading programs from file/string
- Converting to C++ representation
- Converting to frontend AST
- Constraint-checking
'''


def load_program(fname):
	"""
	Load a program from file, and return the parse tree. Includes are resolved.
	:param str fname:
	:rtype: lambdaparser.Program
	:return:
	"""
	code = lambdaparser.load_file_with_includes(fname)
	return lambdaparser.parse_lambda_code(lambdaparser.strip_comments(code))

def load_program_from_string(code):
	"""
	Load a program from file, and return the parse tree. Includes are not resolved.
	:param str|unicode code:
	:rtype: lambdaparser.Program
	:return:
	"""
	if isinstance(code, str):
		code = code.decode('utf-8')
	return lambdaparser.parse_lambda_code(lambdaparser.strip_comments(code))



def translate_file_to_cpp(filename, output_filename=None):
	"""
	Converts a program file to a C++ include file (.lbl.h).
	:param str filename:
	:param str output_filename: (optional) default is filename+'.h'
	"""
	program = load_program(filename)
	cpp_program = program.to_cpp()
	if output_filename is None:
		output_filename = filename+'.h'
	with open(output_filename, 'wb') as f:
		f.write(cpp_program.encode('utf-8'))
	print('File "{}" written.'.format(output_filename))



def check_constraints_on_def(definition):
	"""
	Check the constraints of a single definition
	:param ast.AstNode definition:
	:return: checker.ConstraintCheckResult
	"""
	return checker.check_definition(definition)

def check_constraints_on_program(program):
	"""
	Check all definitions from a program (given as parse tree)
	:param lambdaparser.Program program:
	:rtype: dict[str, checker.ConstraintCheckResult]
	:return:
	"""
	results = OrderedDict()
	for name, definition in program.defs_to_ast():
		results[name] = check_constraints_on_def(definition)  # type: checker.ConstraintCheckResult
	return results




def main_progress(filename):
	"""
	Load a file, check constraints on all definitions, and convert it to a C++ include file afterwards.
	:param str filename:
	"""
	program = load_program(filename)  # type: lambdaparser.Program
	definitions = program.to_ast()  # type: list[(str, ast.AstNode)]
	print filename, 'includes', len(definitions), 'definitions / assumptions'

	# perform constraint checking
	results = check_constraints_on_program(program)
	for name, result in results.items():
		print '---', name, '---'
		result.print_report()

	# finally transform this program for the C++ CoC
	translate_file_to_cpp(filename)


def main():
	# see this as an API / features demo
	if len(sys.argv) <= 1:
		for filename in ['programs/matrixmultiplication.lbl', 'programs/lu_decomposition.lbl']:
			main_progress(filename)
	else:
		for filename in sys.argv[1:]:
			main_progress(filename)


if __name__ == '__main__':
	main()
