import ast
import islpy as isl


"""
- search all nominal-lambda nodes
- bind all outer params "somehow"
- type-infer all nominal-lambda nodes
- type-check all nominal lambda bodies (must be true valid / no error set)
- type-check given node
"""


def find_all_nominals(node, nominals=None):
	"""
	:param node:
	:param nominals:
	:rtype: list[ast.LambdaNominal]
	:return:
	"""
	if nominals is None:
		nominals = []
	if isinstance(node, ast.AstNode):
		if isinstance(node, ast.LambdaNominal):
			if node in nominals:
				return nominals
			nominals.append(node)
		for op in node.ops:
			find_all_nominals(op, nominals)
	return nominals


def manual_typing(node):
	if node.name == 'f1':
		node.cstr_vars = [['i'], ['o']]
		node.cstr_accepted = isl.BasicSet.read_from_str(ast.ctx, '{[i, o] : 0 <= i <= 10}')
		node.cstr_possible = isl.BasicSet.read_from_str(ast.ctx, '{[i, o] : i = o}')


def type_inference(nodes):
	"""
	:param list[ast.LambdaNominal] nodes:
	:return:
	"""
	for node in nodes:
		manual_typing(node)
	# filter out functions that are already types
	nodes = [node for node in nodes if node.cstr_vars is None]
	#TODO some fancy heuristic
	# permissive-type those that have not been typed yet
	for func in nodes:
		if func.cstr_vars is None:
			print '[WARN] No explicit constraints given for '+repr(func)+'. Using permissive default.'
			func.set_default_constraints()


def check_nominal(node):
	"""
	:param ast.LambdaNominal node:
	:return:
	"""
	print 'Checking', node
	print 'matching tmpl', node.cstr_vars, node.cstr_accepted, node.cstr_possible
	body_vars, body_accepted, body_possible = node.get_constraints_recursive()
	print 'against body ', body_vars, body_accepted, body_possible
	# check if body_vars matches node.cstr_vars => fill var_mapping
	var_mapping = {}
	if not ast.check_variables_structure_equal(body_vars, node.cstr_vars, var_mapping):
		raise Exception('Invalid type (vnames) of '+repr(node))
	# check if accepted superset of node.cstr_accepted
	subset, left, right = ast.is_subset(node.cstr_accepted, body_accepted, var_mapping)
	if not subset:
		valid_set, error_set = ast.valid_input_constraints(ast.flatten(node.cstr_vars), right, left)
		print '[ERR]', node, 'violates its given constraints (accepted range)'
		print 'Valid if:  ', ast.simplify_set(valid_set)
		print 'Invalid if:', ast.simplify_set(error_set)
		return False
	# check if (possible intersect node.cstr_accepted) subset of node.cstr_possible
	_, cstr_accepted, possible_ext = ast.is_subset(node.cstr_accepted, body_possible, var_mapping) # bring body_possible to common form
	subset, left, right = ast.is_subset(cstr_accepted.intersect(possible_ext), node.cstr_possible)
	if not subset:
		valid_set, error_set = ast.valid_input_constraints(ast.flatten(node.cstr_vars), right, left)
		print '[ERR]', node, 'violates its given constraints (possible range)'
		print 'Valid if:  ', ast.simplify_set(valid_set)
		print 'Invalid if:', ast.simplify_set(error_set)
		return False
	return True



def check_definition(node):
	"""
	:param ast.AstNode node:
	:return:
	"""
	nominals = find_all_nominals(node)
	print 'Nominal lambdas:', nominals
	type_inference(nominals)
	# Check all nominal lambda bodies
	for node in nominals:
		if not check_nominal(node):
			print '[ERR]', node, 'violates its constraints!'
			return False
		print '[INFO] Recursive function', node, 'has been verified.'
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
