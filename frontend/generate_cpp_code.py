#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import unicode_literals, print_function
import sys
import io
from parser import parse_lambda_code



def translate_file_to_cpp(fname):
	with io.open(fname, encoding='utf-8') as f:
		program = f.read()
	program = parse_lambda_code(program)
	cpp_program = program.to_cpp()
	with io.open(fname+'.h', 'wt', encoding='utf-8') as f:
		f.write(cpp_program)
	print('File "{}" written.'.format(fname+'.h'))



if __name__ == '__main__':
	for param in sys.argv[1:]:
		translate_file_to_cpp(param)

