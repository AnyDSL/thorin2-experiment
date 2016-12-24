import islpy as isl
import ast
import time
import sys


def derive_constraints_iterative(nominals):
	"""
	:param list[ast.LambdaNominal] nominals:
	"""
	print '--- iterative constraint inference ---'
	# first type "permissive"
	for node in nominals:
		node.set_default_constraints()
		print node, 'default', node.cstr_vars, node.cstr_accepted
	# validate and store expected constraints
	for node in nominals:
		print '-- Iteration on', node, '--'
		t = time.time()
		body_vars, body_accepted, body_possible = node.get_constraints_recursive()
		print body_vars, body_accepted
		# Exact simplification gets way too slow - use projection as an approximation for now
		# body_accepted, body_possible = ast.simplify_equalities(body_vars + node.get_outer_vars(), body_accepted, body_possible)

		preserved_vars = set(ast.flatten(body_vars + node.get_outer_vars()))
		missing_vars = preserved_vars.difference(set(body_accepted.get_var_names(isl.dim_type.set)))
		dims = body_accepted.dim(isl.dim_type.set)
		for i in xrange(dims-1, -1, -1):
			v = body_accepted.get_dim_name(isl.dim_type.set, i)
			if v not in preserved_vars:
				body_accepted = body_accepted.project_out(isl.dim_type.set, i, 1)
				body_possible = body_possible.project_out(isl.dim_type.set, i, 1)
		if len(missing_vars) > 0:
			dims = body_accepted.dim(isl.dim_type.set)
			body_accepted = body_accepted.add_dims(isl.dim_type.set, len(missing_vars))
			body_possible = body_possible.add_dims(isl.dim_type.set, len(missing_vars))
			for i, v in enumerate(missing_vars):
				body_accepted = body_accepted.set_dim_name(isl.dim_type.set, dims+i, v)
				body_possible = body_possible.set_dim_name(isl.dim_type.set, dims+i, v)

		print 'simply:', body_accepted, body_possible
		if not body_accepted.is_empty() or body_possible.is_empty():
			node.cstr_vars = body_vars
			node.cstr_accepted = body_accepted
			node.cstr_possible = body_possible
		else:
			print 'Derived constraints are broken (empty)'
		t = time.time() - t
		print 'TIME:', t
	#sys.exit(0)
	print '--- iterative constraint inference finished ---'
