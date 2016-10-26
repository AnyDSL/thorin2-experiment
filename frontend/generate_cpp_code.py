#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import unicode_literals, print_function
import sys
from parser import parse_lambda_code



def translate_file_to_cpp(fname):
	with open(fname, 'r') as f:
		program = f.read().decode('utf-8')
	program = parse_lambda_code(program)
	cpp_program = program.to_cpp()
	with open(fname+'.h', 'w') as f:
		f.write(cpp_program.encode('utf-8'))
	print('File "{}" written.'.format(fname+'.h'))



if __name__ == '__main__':
	for param in sys.argv[1:]:
		translate_file_to_cpp(param)

