# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
from lambdaparser import parse_lambda_code
import frontend


class TestCppCodeGen(unittest.TestCase):
	def test_matrix_multiplication(self):
		program = frontend.load_program('../programs/matrixmultiplication.lbl')
		cpp = program.to_cpp()
		self.assertEqual(cpp.count('auto '), 21)

	def test_lu_decomposition(self):
		program = frontend.load_program('../programs/lu_decomposition.lbl')
		cpp = program.to_cpp()
		self.assertEqual(cpp.count('auto '), 38)


if __name__ == '__main__':
	unittest.main()
