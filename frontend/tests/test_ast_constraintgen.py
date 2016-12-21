# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
from lambdaparser import parse_lambda_code
import ast


class TestAstConstraintGeneration(unittest.TestCase):
	def test_something(self):
		self.assertEqual(True, False)


if __name__ == '__main__':
	unittest.main()
