import islpy as isl

'''
assume SimpleArrGet: pi n:Nat. pi i:Nat. Float; // if 0<=i<n
App1 = SimpleArrGet 2 1;
Wrapper1 = lambda x:Nat. SimpleArrGet 10 x;
'''


last_var_number = 0
def getVarName(name = ''):
	global last_var_number
	last_var_number += 1
	return str(name)+'_'+str(last_var_number)



ctx = isl.DEFAULT_CONTEXT

# type SimpleArrGet
def typeSimpleArrGet():
	n = getVarName('n')
	i = getVarName('i')
	space = isl.Space.create_from_names(ctx, set=[n, i])
	bset = isl.BasicSet.universe(space)
	# const1 + coeff_1*var_1 +... >= 0.
	bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {n: 1}))
	bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {i: 1}))
	bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {n: 1, i: -1, 1: -1}))
	return ({'n': n, 'i': i}, {}, bset)

def typeAddNat():
	a = getVarName('a')
	b = getVarName('b')
	c = getVarName('a+b')
	space = isl.Space.create_from_names(ctx, set=[a, b, c])
	bset = isl.BasicSet.universe(space)
	bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {a: 1}))
	bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {b: 1}))
	bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {c: 1}))
	bset = bset.add_constraint(isl.Constraint.eq_from_names(space, {a: 1, b: 1, c: -1}))
	return ({'a': a, 'b': b}, {'c': c}, bset)

def typeNatConst(n):
	c = getVarName('c'+str(n))
	space = isl.Space.create_from_names(ctx, set=[c])
	bset = isl.BasicSet.universe(space)
	bset = bset.add_constraint(isl.Constraint.eq_from_names(space, {c: 1, 1: -n}))
	return ({}, {'c': c}, bset)


def compress(v_in, v_out, v_set):

	return (v_in, v_out, v_set)


def typeApplication(func_type, param_binding, param_types):
	f_in, f_out, f_set = func_type
	# join sets
	for p_in, p_out, p_set in param_types:
		f_set = f_set.flat_product(p_set)
		for k,v in p_in:
			f_in[k] = v
	# "bind" params
	for varname, value in param_binding.items():
		f_set = f_set.add_constraint(isl.Constraint.eq_from_names(f_set.space, {f_in[varname]: 1, value: -1}))
		del f_in[varname]
	# determine in/out parameters
	# compress set
	#return compress(f_in, f_out, f_set)
	return (f_in, f_out, f_set)





print 'SimpleArrGet:',typeSimpleArrGet()
print 'AddNat:',typeAddNat()
print '1:', typeNatConst(1)
print

# Type 1 + 2
type1 = typeNatConst(1)
type2 = typeNatConst(2)
print '1+2 :', typeApplication(typeAddNat(), {'a': type1[1]['c'], 'b': type2[1]['c']}, [type1, type2])
print

# Type ArrGet 2 1
type2 = typeNatConst(2)
print 'SimpleArrGet 2: ', typeApplication(typeSimpleArrGet(), {'n': type2[1]['c']}, [type2])
type1 = typeNatConst(1)
type2 = typeNatConst(2)
print 'SimpleArrGet 2 1: ', typeApplication(typeApplication(typeSimpleArrGet(), {'n': type2[1]['c']}, [type2]), {'i': type1[1]['c']}, [type1])
print

space = isl.Space.create_from_names(ctx, set=["v1", "v2"])
bset = isl.BasicSet.universe(space)
bset = bset.add_constraint(isl.Constraint.eq_from_names(space, {"v1": 1, "v2": -1}))
space2 = isl.Space.create_from_names(ctx, set=["v1", "v3"])
bset2 = isl.BasicSet.universe(space2)
bset2 = bset2.add_constraint(isl.Constraint.eq_from_names(space2, {"v1": 1, "v3": -1}))
print space, bset
print space2, bset2
bset3 = bset.flat_product(bset2)
print bset3.space, bset3
print





x = isl.Space.set_alloc(ctx, 1, 0)
x = isl.BasicSet.universe(x)
print x

space = isl.Space.create_from_names(ctx, set=["x", "y"])

bset = (isl.BasicSet.universe(space)
		.add_constraint(isl.Constraint.ineq_from_names(space, {1: -1, "x": 1}))
        .add_constraint(isl.Constraint.ineq_from_names(space, {1: 5, "x": -1}))
        .add_constraint(isl.Constraint.ineq_from_names(space, {1: -1, "y": 1}))
        .add_constraint(isl.Constraint.ineq_from_names(space, {1: 5, "y": -1})))
print "set 1 %s:" % bset

bset2 = isl.BasicSet("{[x, y] : x >= 0 and x < 5 and y >= 0 and y < x+4 }", context=ctx)
print "set 2: %s" % bset2

bsets_in_union = []
bset.union(bset2).convex_hull().foreach_basic_set(bsets_in_union.append)
print bsets_in_union
union, = bsets_in_union
print "union: %s" % union

