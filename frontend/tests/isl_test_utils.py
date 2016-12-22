import ast
import islpy as isl


def assert_isl_sets_equal(self, set1, set2, equal_vars=None, allow_set1_additional_vars=False, allow_set2_additional_vars=False):
	if isinstance(set1, basestring):
		set1 = ast.read_set_from_string(set1)
	if isinstance(set2, basestring):
		set2 = ast.read_set_from_string(set2)
	if equal_vars is None:
		equal_vars = {}

	# check  set1 subset of set2
	isok, _, newset = ast.is_subset(set1, set2, equal_vars)
	if not isok:
		self.fail('set1 is larger than set2:\nset1 = '+str(set1)+'\nset2 = '+str(set2))

	# check  set2 subset of set1
	isok, _, newset = ast.is_subset(set2, set1, equal_vars)
	if not isok:
		self.fail('set1 is smaller than set2:\nset1 = '+str(set1)+'\nset2 = '+str(set2))

	# check vars1 subset of vars2
	vars1 = set([equal_vars[vname] if vname in equal_vars else vname for vname in set1.get_var_names(isl.dim_type.set)])
	vars2 = set(set2.get_var_names(isl.dim_type.set))
	if not allow_set1_additional_vars:
		diff = vars1.difference(vars2)
		if len(diff) > 0:
			self.fail('set1 has ' + str(len(diff)) + ' additional vars: '+', '.join(diff))

	# check vars2 subset of vars1
	if not allow_set2_additional_vars:
		diff = vars2.difference(vars1)
		if len(diff) > 0:
			self.fail('set2 has ' + str(len(diff)) + ' additional vars: ' + ', '.join(diff))

