import ast
import islpy as isl


"""
- search all nominal-lambda nodes
- bind all outer params "somehow"
- type-infer all nominal-lambda nodes
- type-check all nominal lambda bodies (must be true valid / no error set)
- type-check given node
"""


def find_all_nominals(node, nominals=[]):
	'''
	:param node:
	:param nominals:
	:rtype: List(ast.LambdaNominal)
	:return:
	'''
	if isinstance(node, ast.AstNode):
		if isinstance(node, ast.LambdaNominal):
			if node in nominals:
				return nominals
			nominals.append(node)
		for op in node.ops:
			find_all_nominals(op, nominals)
	return nominals


def type_inference(nodes):
	'''
	:param List(ast.LambdaNominal) nodes:
	:return:
	'''
	# filter out functions that are already types
	nodes = [node for node in nodes if node.cstr_vars is None]
	#TODO some fancy heuristic
	# permissive-type those that have not been typed yet
	for func in nodes:
		if func.cstr_vars is None:
			print '[WARN] No explicit constraints given for '+repr(func)
			func.cstr_vars = func.ops[1].get_nat_variables()
			bset = isl.BasicSet.universe(isl.Space.create_from_names(ast.ctx, set=ast.flatten(func.cstr_vars)))
			func.cstr_accepted = bset
			func.cstr_possible = bset


def check_nominal(node):
	'''
	:param ast.LambdaNominal node:
	:return:
	'''
	print 'Checking', node
	print 'matching', node.cstr_vars, node.cstr_accepted, node.cstr_possible
	body_vars, body_accepted, body_possible = node.get_constraints_recursive()
	# check if body_vars matches node.cstr_vars
	# check if accepted superset of node.cstr_accepted
	# check if (possible intersect node.cstr_accepted) subset of node.cstr_possible


def check_definition(node):
	'''
	:param ast.AstNode node:
	:return:
	'''
	nominals = find_all_nominals(node)
	print 'Nominal lambdas:', nominals
	type_inference(nominals)
	# Check all nominal lambda bodies
	for node in nominals:
		if not check_nominal(node):
			print '[ERR]', node, 'violates its constraints!'
			return False
	# check "real" program - assuming all recursive functions are safe
	return check_definition_simple(node)


def check_definition_simple(node):
	vars, accepted, possible = node.get_constraints()
	print 'Vars:    ',vars
	print 'Accepted:',accepted
	print 'Possible:',possible
	# possible subsetof accepted => everything right
	if possible.is_subset(accepted):
		print '[VALID]'
		return True
	# possible can't be made accepted by other constraints
	if possible.is_disjoint(accepted):
		print '[INVALID]  (disjoint)'
		return False
	unbound_vars = ast.flatten(vars)
	# any outer influence on the breaking parts?
	if len(unbound_vars) == 0:
		print '[INVALID]  (not satisfied, no influence on constraints remaining)'
		return False
	# constraints on "valid" inputs
	print '[???]'
	valid_set, error_set = ast.valid_input_constraints(unbound_vars, accepted, possible)
	print 'Valid if:  ', ast.simplify_set(valid_set)
	print 'Invalid if:', ast.simplify_set(error_set)
