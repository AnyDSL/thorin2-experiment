import ast
import islpy as isl
import constraint_derivation
import predefined_functions


"""
- search all nominal-lambda nodes
- bind all outer params "somehow"
- type-infer all nominal-lambda nodes
- type-check all nominal lambda bodies (must be true valid / no error set)
- type-check given node
"""



class ConstraintCheckResult:
	"""
	The result of a constraint check is always one of:
	"valid" (in every case)
	"invalid" (in every case)
	"partial" (can be valid, depending on the input
	In case of partial validity, there is a "valid_set" (valid inputs) and an "error_set" (invalid inputs).
	"""
	def __init__(self, valid, invalid, vars=None, accepted=None, possible=None, error_set=None, valid_set=None, msg=''):
		self.valid = valid
		self.invalid = invalid
		self.vars = vars
		self.accepted = accepted
		self.possible = possible
		self.error_set = error_set
		self.valid_set = valid_set
		self.msg = msg

	def is_valid(self):
		return self.valid

	def is_invalid(self):
		return self.invalid

	def is_partially_valid(self):
		return not self.valid and not self.invalid

	def print_short_report(self):
		if self.valid:
			print '[VALID]'
		elif self.invalid:
			print '[INVALID]'
		else:
			print '[PARTIAL]'

	def print_report(self):
		if self.valid:
			print '[VALID]', self.msg
		elif self.invalid:
			print '[INVALID]', self.msg
		else:
			print '[PARTIAL]', self.msg
			print 'Variables:  ', self.vars
			print 'Valid if:   ', ast.simplify_set(self.valid_set)
			print 'Invalid if: ', ast.simplify_set(self.error_set)

	def __str__(self):
		if self.valid:
			return 'CCR<[VALID]>'
		elif self.invalid:
			return 'CCR<[INVALID]>'
		else:
			return 'CCR<[PARTIAL] (error '+str(self.error_set)+')>'


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


def type_inference(nodes):
	"""
	:param list[ast.LambdaNominal] nodes:
	:return:
	"""
	# filter out functions that are already constrained
	nodes = [node for node in nodes if node.cstr_vars is None]
	# some dumb heuristic
	if len(nodes) > 0:
		constraint_derivation.derive_constraints_iterative(nodes)
	# permissive-type those that have not been typed yet
	for func in nodes:
		if func.cstr_vars is None:
			print '[WARN] No explicit constraints given for '+repr(func)+'. Using (permissive) default:'
			func.set_default_constraints()
			print func.cstr_vars, func.cstr_accepted, func.cstr_possible


def check_nominal(node):
	"""
	:param ast.LambdaNominal node:
	:return:
	"""
	print '--- Checking', node, '---'
	# First: all parameter variables have to be present (including unbound ones)
	required_vars = ast.flatten(node.cstr_vars)
	for param in node.get_outer_parameters():
		required_vars += ast.flatten(param.get_default_vars())
	present_vars_accepted = ast.set_vars(node.cstr_accepted)
	present_vars_possible = ast.set_vars(node.cstr_possible)
	assert present_vars_accepted == present_vars_possible
	for vname in required_vars:
		if not vname in present_vars_accepted:
			raise Exception('Invalid constraint template! Variable "{}" missing'.format(vname))
	# Next, constraints have to match
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
		valid_set, error_set = ast.valid_input_constraints(ast.flatten(node.cstr_vars) + ast.flatten(node.get_outer_vars()), right, left)
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



# That's the method you want to call!    <=====
def check_definition(root):
	"""
	:param ast.AstNode root:
	:return:
	"""
	nominals = find_all_nominals(root)
	type_inference(nominals)
	# Check all nominal lambda bodies
	for node in nominals:
		if not check_nominal(node):
			return ConstraintCheckResult(False, True, msg='Invalid sub-function: '+str(node))
		print '[INFO] Recursive function', node, 'has been verified.'
	# check "real" program - assuming all recursive functions are safe
	if len(nominals) > 0:
		print '--- checking root ---'
	return check_definition_simple(root)


def check_definition_simple(node):
	vars, accepted, possible = node.get_constraints()
	#accepted2, possible2 = ast.simplify_equalities(vars, accepted, possible)
	#print 'Vars:    ',vars
	#print 'Accepted:',accepted2
	#print 'Possible:',possible2
	# possible subsetof accepted => everything right
	unbound_vars = ast.flatten(vars)
	valid_set, error_set = ast.valid_input_constraints(unbound_vars, accepted, possible)
	if possible.is_subset(accepted):
		print '[VALID]'
		return ConstraintCheckResult(True, False, vars, accepted, possible, error_set, valid_set)
	# possible can't be made accepted by other constraints
	if possible.is_disjoint(accepted):
		print '[INVALID]  (disjoint)'
		return ConstraintCheckResult(False, True, vars, accepted, possible, error_set, valid_set)
	# any outer influence on the breaking parts?
	if len(unbound_vars) == 0:
		print '[INVALID]  (not satisfied, no influence on constraints remaining)'
		return ConstraintCheckResult(False, True, vars, accepted, possible, error_set, valid_set,
									 msg='not satisfied, no influence on constraints remaining')
	# constraints on "valid" inputs
	return ConstraintCheckResult(False, False, vars, accepted, possible, error_set, valid_set,
								 msg='valid only for specific input')
