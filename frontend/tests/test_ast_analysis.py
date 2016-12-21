# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
import ast
from lambdaparser import parse_lambda_code


def ast_from_expr(expr, assumes = ''):
	"""
	:param str expr:
	:param str assumes:
	:rtype: ast.AstNode
	:return:
	"""
	return parse_lambda_code(assumes + ' define d = ' + expr + ';').to_ast()[-1][1]




class TestAstTyping(unittest.TestCase):
	def test_assume_param_typing(self):
		node = ast_from_expr('test', 'assume test: Nat->Nat;').get_type()
		self.assertEqual(unicode(node), 'Π_:Nat. Nat')
		node = ast_from_expr('15').get_type()
		self.assertEqual(unicode(node), 'Nat')
		node = ast_from_expr('lambda x:Nat. x').get_type()
		self.assertEqual(unicode(node), 'Πx:Nat. Nat')
		node = ast_from_expr('(lambda x:Nat. x) 0').get_type()
		self.assertEqual(unicode(node), 'Nat')

	def test_lambda_typing(self):
		node = ast_from_expr('lambda t:*. lambda x1:t. lambda op:(pi _:t. t). op(op(x1))').get_type()
		self.assertEqual(unicode(node), 'Πt:*. Πx1:t. Πop:Π_:t. t. t')



if __name__ == '__main__':
	unittest.main()
