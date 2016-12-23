# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import unittest
from lambdaparser import parse_lambda_code
import ast
import islpy as isl


class TestUtilFunctions(unittest.TestCase):
	from isl_test_utils import assert_isl_sets_equal, assert_constraints_equal

	def test_get_var_name(self):
		v1 = ast.get_var_name('xyz')
		v2 = ast.get_var_name('xyz')
		self.assertNotEqual(v1, v2)
		self.assertTrue(v1.startswith('xyz'))
		self.assertTrue(v2.startswith('xyz'))

	def test_basic_set_join_python(self):
		ab = ast.read_set_from_string('{[a,b] : a >= 0 and b >= 2}')
		ac = ast.read_set_from_string('{[a,c] : a <= 10 and c >= 1}')
		de = ast.read_set_from_string('{[d,e] : d < 0}')
		self.assert_isl_sets_equal(ast.basic_set_join_python(ab, ac), '{[a,b,c] : 0 <= a <= 10 and b >= 2 and c >= 1}')
		self.assert_isl_sets_equal(ast.basic_set_join_python(ab, de), '{[a,b,d,e] : 0 <= a and b >= 2 and d < 0}')
		self.assert_isl_sets_equal(ast.basic_set_join_python(ab, ab), ab)

	def test_set_join(self):
		ab = ast.read_set_from_string('{[a,b] : a >= 0 or b >= 2}')
		ac = ast.read_set_from_string('{[a,c] : a <= 10 or c >= 1}')
		de = ast.read_set_from_string('{[d,e] : d < 0}')
		self.assert_isl_sets_equal(ast.set_join(ab, ac), '{[a,b,c] : (0 <= a or b >= 2) and (a <= 10 or c >= 1)}')
		self.assert_isl_sets_equal(ast.set_join(ab, de), '{[a,b,d,e] : (0 <= a or b >= 2) and d < 0}')
		self.assert_isl_sets_equal(ast.set_join(ab, ab), ab)

	def test_clone_constraint(self):
		c1 = isl.Constraint.ineq_from_names(isl.Space.create_from_names(ast.ctx, ['n', 'a']), {'n': 1, 1: -10})
		c2 = isl.Constraint.ineq_from_names(isl.Space.create_from_names(ast.ctx, ['c', 'n', 'b']), {'n': 1, 1: -10})
		c12 = ast.clone_constraint(c2.space, c1)
		self.assert_constraints_equal(c12, c2)

		c1 = isl.Constraint.eq_from_names(isl.Space.create_from_names(ast.ctx, ['n', 'a']), {'n': 1, 1: -10})
		c2 = isl.Constraint.eq_from_names(isl.Space.create_from_names(ast.ctx, ['c', 'n', 'b']), {'n': 1, 1: -10})
		c12 = ast.clone_constraint(c2.space, c1)
		self.assert_constraints_equal(c12, c2)

	def test_var_in_set(self):
		bset = ast.read_set_from_string('{[a,b,ccc] : }')
		self.assertTrue(ast.var_in_set('a', bset))
		self.assertTrue(ast.var_in_set('ccc', bset))
		self.assertFalse(ast.var_in_set('d', bset))

	def test_set_vars(self):
		bset = ast.read_set_from_string('{[a,b,ccc] : }')
		self.assertEqual(ast.set_vars(bset), ['a', 'b', 'ccc'])

	def test_add_variables(self):
		bset = ast.read_set_from_string('{[a,b,c] : a >= 0 and b >= 1}')
		bset2 = ast.add_variables(bset, ['a', 'c', 'd'])
		self.assertEqual(bset2.get_var_names(isl.dim_type.set), ['a', 'b', 'c', 'd'])
		self.assert_isl_sets_equal(bset, bset2, allow_set2_additional_vars=True)

	def test_constraint_equal(self):
		bset = ast.constraint_equal(ast.read_set_from_string('{[a,b,c,d] : a >= 0 and d < 0}'), 'a', 'b', 'c')
		self.assert_isl_sets_equal(bset, '{[a,b,c,d] : a = b = c >= 0 and d < 0}')

	def test_constraint_vars_equal(self):
		bset = ast.constraint_vars_equal(ast.read_set_from_string('{[a,b,d] : a >= 0 and d < 0}'), [[], ['a']], [[], ['b']])
		self.assert_isl_sets_equal(bset, '{[a,b,d] : a = b >= 0 and d < 0}')

	def test_constraint_inverse(self):
		bset = ast.read_set_from_string('{[a,b] : a+b >= 15}')
		cr = ast.constraint_inverse(bset.get_constraints()[0])
		bset2 = isl.BasicSet.universe(bset.space).add_constraint(cr)
		self.assert_isl_sets_equal(bset2, '{[a,b] : a+b < 15}')

	def test_flatten(self):
		self.assertEqual(ast.flatten([[['a'], [], [[]]], ['b', 'c', [[]]]]), ['a', 'b', 'c'])

	def test_simplify_set(self):
		bset = ast.simplify_set(ast.read_set_from_string('{[a,b,c] : a > 0 and a > 1 and a > 2 and a < 10 and b = 0 and b < 1}'))
		self.assertEqual(str(bset), '{ [a, b = 0, c] : 3 <= a <= 9 }')

	def test_get_equivalence_classes(self):
		eqclasses =  ast.get_equivalence_classes(ast.read_set_from_string('{[a,b,c,d,e,f] : a = b and c = d = e}'))
		self.assertEqual(eqclasses['a'], set(['a', 'b']))
		self.assertEqual(eqclasses['b'], set(['a', 'b']))
		self.assertIs(eqclasses['a'], eqclasses['b'])
		self.assertEqual(eqclasses['c'], set(['c', 'd', 'e']))
		self.assertEqual(eqclasses['d'], set(['c', 'd', 'e']))
		self.assertEqual(eqclasses['e'], set(['c', 'd', 'e']))
		self.assertEqual(eqclasses['f'], set(['f']))

	def test_get_constrainted_variables(self):
		vars = ast.get_constrainted_variables(ast.read_set_from_string('{[a,b,c] : a >= 0 and b = 1}'))
		self.assertEqual(vars, set(['a', 'b']))

	def test_simplify_equalities(self):
		bset,  = ast.simplify_equalities(['b', 'd'], ast.read_set_from_string('{[a,b,c,d,e] : a = b = c and e > 0}'))
		self.assertEqual(bset.get_var_names(isl.dim_type.set), ['b', 'd', 'e'])
		self.assert_isl_sets_equal(bset, '{ [b, d, e] : e > 0 }')

	def test_clone_variables(self):
		vars = ['a', [['b'], ['c'], [[]]]]
		vars2 = ast.clone_variables(vars, {'c': 'd'})
		self.assertTrue(ast.check_variables_structure_equal(vars, vars2))
		x, y, d = ast.flatten(vars2)
		self.assertNotEqual(x, 'a')
		self.assertNotEqual(y, 'b')
		self.assertEqual(d, 'd')

	def test_check_variables_structure_equal(self):
		vars  = ['a', [['b'], ['c'], [[]]]]
		vars2 = ['d', [['e'], ['f'], [[]]]]    # ==
		vars3 = ['d', [['e'], ['f'], [[[]]]]]  # !=
		d = {}
		self.assertTrue(ast.check_variables_structure_equal(vars, vars2, d))
		self.assertEqual(d['a'], 'd')
		self.assertEqual(d['b'], 'e')
		self.assertEqual(d['c'], 'f')
		self.assertFalse(ast.check_variables_structure_equal(vars, vars3))

	def test_valid_input_constraints(self):
		accepted = ast.read_set_from_string('{[a, b] : a >= b and a < 10}')
		possible = ast.read_set_from_string('{[a, b] : a = b and b < 11}')
		valid_set, error_set = ast.valid_input_constraints(['a', 'b'], accepted, possible)
		self.assert_isl_sets_equal(valid_set, '{[a,b] : a = b < 10}')  # possible and valid configs
		self.assert_isl_sets_equal(error_set, '{[a,b] : a = b = 10}')  # possible but invalid configs

	def test_set_align_params(self):
		bset1 = ast.read_set_from_string('{[a,b,c] : c > 0}')
		bset2 = ast.read_set_from_string('{[c,b,a] : c > 0}')
		self.assertEqual(bset1.get_var_names(isl.dim_type.set), ['a', 'b', 'c'])
		self.assertEqual(bset2.get_var_names(isl.dim_type.set), ['c', 'b', 'a'])
		bset3 = ast.set_align_params(bset2, bset1.space)
		self.assertEqual(bset3.get_var_names(isl.dim_type.set), ['a', 'b', 'c'])
		self.assertEqual(bset1, bset3)

	def test_is_subset(self):
		bset1 = ast.read_set_from_string('{[a, b] : a >= 1}')
		bset2 = ast.read_set_from_string('{[a, c] : a >= 0 and c > 0}')
		eq, s1, s2 = ast.is_subset(bset1, bset2)
		self.assertTrue(eq)
		self.assertEqual(bset1, s1)
		self.assertEqual(s2.get_var_names(isl.dim_type.set), ['a', 'b'])

		bset1 = ast.read_set_from_string('{[a, b] : a >= 1 and b = 0}')
		bset2 = ast.read_set_from_string('{[a, c] : a = c and c > 1}')
		eq, s1, s2 = ast.is_subset(bset1, bset2)
		self.assertFalse(eq)
		self.assertEqual(bset1, s1)
		self.assertEqual(s2.get_var_names(isl.dim_type.set), ['a', 'b'])

	def test_read_set_from_string(self):
		bset1 = ast.read_set_from_string('{[a,b,c] : a >= 0 and b = 1}')
		bset2 = isl.BasicSet.universe(isl.Space.create_from_names(ast.ctx, ['a', 'b', 'c']))
		bset2 = bset2.add_constraint(isl.Constraint.ineq_from_names(bset2.space, {'a': 1}))
		bset2 = bset2.add_constraint(isl.Constraint.eq_from_names(bset2.space, {'b': 1, 1: -1}))
		self.assertEqual(bset1, bset2)
		self.assertEqual(str(bset1), str(bset2))


if __name__ == '__main__':
	unittest.main()
