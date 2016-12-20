import islpy as isl
import ast
import time
import sys


def derive_constraints_iterative(nominals):
	"""
	:param list[ast.LambdaNominal] nominals:
	:return:
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
		#body_accepted, body_possible = ast.simplify_equalities(body_vars + node.get_outer_vars(), body_accepted, body_possible)
		print 'simply:', body_accepted
		#TODO further simplification
		node.cstr_vars = body_vars
		node.cstr_accepted = body_accepted
		node.cstr_possible = body_possible
		t = time.time() - t
		print 'TIME:', t
		#sys.exit(0)
	print '--- iterative constraint inference finished ---'
