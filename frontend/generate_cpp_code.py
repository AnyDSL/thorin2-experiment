#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import unicode_literals, print_function
import sys
import os
from lambdaparser import parse_lambda_code


HELP = 'USAGE: '+__file__+""" [params] [filename...]
Parameters:
--all -a : Progress all files in this directory
"""



def translate_file_to_cpp(fname):
	with open(fname, 'r') as f:
		program = f.read().decode('utf-8')
	program = parse_lambda_code(program)
	cpp_program = program.to_cpp()
	with open(fname+'.h', 'w') as f:
		f.write(cpp_program.encode('utf-8'))
	print('File "{}" written.'.format(fname+'.h'))



def translate_all():
	for f in os.listdir('.'):
		if f.endswith('.lbl') and os.path.isfile(f):
			translate_file_to_cpp(f)



if __name__ == '__main__':
	for param in sys.argv[1:]:
		if param == '--all' or param == '-a':
			translate_all()
		else:
			translate_file_to_cpp(param)
	if len(sys.argv) <= 1:
		print(HELP)

