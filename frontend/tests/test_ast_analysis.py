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

	def test_nat_variable_names(self):
		node = ast_from_expr('(x, lambda n:Nat. lambda c:(Nat->Nat). (n,x))', 'assume x: Nat;')
		ref = [['v1'], [['n'], [[['c1'], ['c2']], [['n'], ['v2']]]]]
		# new random names (get_nat_variables)
		result = node.get_nat_variables()
		self.assertTrue(ast.check_variables_structure_equal(result, ref))
		self.assertTrue(set(ast.flatten(result)).isdisjoint(set(ast.flatten(ref))))
		# unique names (get_nat_variables_unique)
		result = node.get_nat_variables_unique(ast.VarNameGen('v'))
		self.assertEqual(ast.flatten(result), ast.flatten(ref))
		self.assertTrue(ast.check_variables_structure_equal(result, ref))
		result2 = node.get_nat_variables_unique(ast.VarNameGen('v'))
		self.assertEqual(result, result2)



if __name__ == '__main__':
	unittest.main()
