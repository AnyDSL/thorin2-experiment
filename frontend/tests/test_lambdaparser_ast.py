# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
from lambdaparser import parse_lambda_code
import ast


class TestLambdaParserAstGeneration(unittest.TestCase):
	from isl_test_utils import assert_isl_sets_equal

	def assertExpressionIdentity(self, expr, assumes=''):
		"""
		Parse the expression (as single definition) and convert it to its AST node.
		The generated AST is converted to string and compared to the input expression.
		:param unicode|str expr:
		:param unicode|str assumes: Additional assumed entities
		:rtype: ast.AstNode
		:return: the generated ast node
		"""
		program = parse_lambda_code(assumes + ' define d = '+expr+';').to_ast()
		serialized = unicode(program[-1][1])
		ref = expr.replace('lambda ', 'λ').replace('pi ', 'Π').replace('sigma(', 'Σ(')
		self.assertEqual(serialized, ref)
		return program[-1][1]

	def test_empty(self):
		astnode = parse_lambda_code('').to_ast()
		self.assertIsInstance(astnode, list)
		self.assertEqual(len(astnode), 0)

	def test_parameter_binding(self):
		node = self.assertExpressionIdentity('lambda x:Nat. x')
		self.assertEqual(node.ops[0], node.ops[1])
		# unnamed parameter
		self.assertExpressionIdentity('lambda _:Nat. *')
		# unbound parameter
		self.assertRaises(Exception, lambda: parse_lambda_code('define f = lambda x:Nat. y;').to_ast())
		# multiple parameters with same name
		node = self.assertExpressionIdentity('lambda a:Nat. lambda b:Nat. lambda a:Nat. (a, b)')
		a1 = node.ops[0]
		a2 = node.ops[1].ops[1].ops[0]
		b = node.ops[1].ops[0]
		ret = node.ops[1].ops[1].ops[1].ops
		self.assertEqual(a1.name, 'a')
		self.assertEqual(a2.name, 'a')
		self.assertEqual(b.name, 'b')
		self.assertEqual(len(ret), 2)
		self.assertEqual(ret[0], a2)
		self.assertEqual(ret[1], b)
		self.assertNotEqual(a1, a2)

	def test_auto_assumes(self):
		node = self.assertExpressionIdentity('0')
		self.assertIsInstance(node, ast.Assume)
		self.assertEqual(str(node.get_type()), 'Nat')
		node = parse_lambda_code('define c = cNat0;').to_ast()[0][1]
		self.assertIn(str(node), ['0', 'cNat0'])
		self.assertIsInstance(node, ast.Assume)
		self.assertEqual(str(node.get_type()), 'Nat')

	def test_tuple_sigma(self):
		# tuple
		node = self.assertExpressionIdentity('(0, 1)')
		self.assertIsInstance(node, ast.Tupel)
		# sigma (type of tuple)
		node = self.assertExpressionIdentity('sigma(0, 1)')
		self.assertIsInstance(node, ast.Sigma)
		# extracting from tuple
		node = self.assertExpressionIdentity('(0, 1)[0]')
		self.assertIsInstance(node, ast.Extract)
		self.assertIsInstance(node.ops[0], ast.Tupel)
		self.assertIsInstance(node.ops[1], int)

	def test_application(self):
		node = self.assertExpressionIdentity('f(0)', 'assume f: Nat -> Nat;')
		self.assertIsInstance(node, ast.App)
		node = self.assertExpressionIdentity('f(0)(1)', 'assume f: Nat -> Nat -> Nat;')
		self.assertIsInstance(node, ast.App)

	def test_lambda(self):
		self.assertExpressionIdentity('lambda t:*. lambda x1:t. lambda op:pi _:t. t. op(op(x1))(x1)')
		node = parse_lambda_code('define d = lambda rec f (x:Nat):Nat. f x;').to_ast()[-1][1]
		self.assertEqual(unicode(node), 'λ rec f(x:Nat): Nat. []')
		self.assertEqual(unicode(node.ops[2]), 'λ rec f(x:Nat): Nat. [](x)')
		# this is perfectly valid
		node = parse_lambda_code('''
		define d = let
			define f1 = lambda rec (x:Nat):Nat. f2 x;
			define fx = lambda rec f2 (y:Nat):Nat. f1 y;
		in (f1, fx) end;
		''').to_ast()[-1][1]
		self.assertIsInstance(node.ops[0], ast.LambdaNominal)
		self.assertIsInstance(node.ops[1], ast.LambdaNominal)
		self.assertEqual(unicode(node), '(λ rec (x:Nat): Nat. [], λ rec f2(y:Nat): Nat. [])')

	def test_matrix_multiplication(self):
		with open('../programs/arrays-simple.lbl', 'rb') as f:
			program = f.read().decode('utf-8')
		program = parse_lambda_code(program).to_ast()
		self.assertEqual(len(program), 13)

	def test_lu_decomposition(self):
		with open('../programs/lu_decomposition-simple.lbl', 'rb') as f:
			program = f.read().decode('utf-8')
		program = parse_lambda_code(program).to_ast()
		self.assertEqual(len(program), 26)


	# === Constraint parsing ===
	def test_constraints_simple(self):
		node = parse_lambda_code('''
		@guarantees "v1 + v2 = v3"
		assume opNatPlus: (Nat,Nat) -> Nat;
		''').to_ast()[-1][1]
		self.assertIsInstance(node, ast.Assume)
		self.assertTrue(ast.check_variables_structure_equal(node.cstr_vars, [[['a'], ['b']], ['c']]))
		self.assert_isl_sets_equal(node.cstr_accepted, '{[v1, v2, v3]: }', allow_set1_additional_vars=True)
		self.assert_isl_sets_equal(node.cstr_possible, '{[v1, v2, v3]: v1 + v2 = v3}', allow_set1_additional_vars=True)

		node = parse_lambda_code('''
		define f =
		@accepts "x > 0"
		@guarantees "x = fx1"
		@guarantees "fx1 >= 0"
		lambda rec fx(x:Nat):Nat. x;
		''').to_ast()[-1][1]
		self.assertIsInstance(node, ast.LambdaNominal)
		translation = {}
		self.assertTrue(ast.check_variables_structure_equal(node.cstr_vars, [['a'], ['b']], translation))
		self.assert_isl_sets_equal(node.cstr_accepted, '{[a, b]: a > 0}', translation, True)
		self.assert_isl_sets_equal(node.cstr_possible, '{[a, b]: a = b >= 0}', translation, True)

		node = parse_lambda_code('''
		@guarantees "x = fx1"
		define f = lambda rec fx(x:Nat):Nat. x;
		''').to_ast()[-1][1]
		self.assertIsInstance(node, ast.LambdaNominal)
		translation = {}
		self.assertTrue(ast.check_variables_structure_equal(node.cstr_vars, [['a'], ['b']], translation))
		self.assert_isl_sets_equal(node.cstr_accepted, '{[a, b]: }', translation, True)
		self.assert_isl_sets_equal(node.cstr_possible, '{[a, b]: a = b}', translation, True)

		node = parse_lambda_code('''
		define d = lambda n:Nat.
			@accepts "0 <= i < n"
			lambda rec (i:Nat):*. (i, n, Nat)[2];
		''').to_ast()[-1][1]
		self.assertIsInstance(node, ast.Lambda)
		translation = {}
		self.assertTrue(ast.check_variables_structure_equal(node.ops[1].cstr_vars, [['a'], []], translation))
		self.assert_isl_sets_equal(node.ops[1].cstr_accepted, '{[n, a]: 0 <= a < n}', translation, True)
		self.assert_isl_sets_equal(node.ops[1].cstr_possible, '{[n, a]: }', translation, True)


	def test_constraints_invalid(self):
		self.assertRaises(Exception, lambda: parse_lambda_code('''
		@abc "v1 + v2 = v3"
		assume opNatPlus: (Nat,Nat) -> Nat;
		''').to_ast())




if __name__ == '__main__':
	unittest.main()