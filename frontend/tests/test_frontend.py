# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
import frontend


class TestFrontend(unittest.TestCase):
	def test_matrix_multiplication(self):
		program = frontend.load_program('../programs/matrixmultiplication.lbl')
		definitions = program.to_ast()

	def test_lu_decomposition(self):
		program = frontend.load_program('../programs/lu_decomposition.lbl')
		definitions = program.to_ast()


if __name__ == '__main__':
	unittest.main()
