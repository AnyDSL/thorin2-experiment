#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import unicode_literals, print_function
import sys
import os
from lambdaparser import parse_lambda_code
import frontend

'''
Converts lambda programs (.lbl files) to C++ include files (.lbl.h).
'''


HELP = 'USAGE: '+__file__+""" [params] [filename...]
Parameters:
--all -a : Progress all files in this directory
"""



def translate_all():
	for f in os.listdir('programs'):
		if f.endswith('.lbl') and os.path.isfile('programs/'+f):
			frontend.translate_file_to_cpp('programs/'+f)



if __name__ == '__main__':
	for param in sys.argv[1:]:
		if param == '--all' or param == '-a':
			translate_all()
		else:
			frontend.translate_file_to_cpp(param)
	if len(sys.argv) <= 1:
		print(HELP)

