import islpy as isl

ctx = isl.DEFAULT_CONTEXT

# check for transitive closure
'''
define test = lambda rec f (x,n:Nat,Nat):Nat. if x < n then 1 + f (x+1, n) else n;
Expected: [[x,n], [result]], {x,n,r}, {x,n,r | r = n}

{x1, n1, r1}   =>   {x2, n2, r2, x1, n1, r2 | x2<n2  & n1=n2 & x1=x2+1 & r2=r1}
                  U {x2, n2, r2, x1, n1, r2 | x2>=n2 & r2=n2}

'''

space = isl.Space.create_from_names(ctx, in_=["x", "n", "r"], out=['x2', 'n2', 'r2'])
m = m2 = isl.Map.universe(space)
m = m.add_constraint(isl.Constraint.eq_from_names(space, {'n2': 1, 'n': -1}))
m = m.add_constraint(isl.Constraint.eq_from_names(space, {'r2': 1, 'r': -1, 1: -1}))
m = m.add_constraint(isl.Constraint.eq_from_names(space, {'x2': 1, 'x': -1, 1: 1}))
m = m.add_constraint(isl.Constraint.ineq_from_names(space, {'n2': 1, 1: -1, 'x2': -1}))

m2 = m2.add_constraint(isl.Constraint.eq_from_names(space, {'r2': 1, 'n2': -1}))
m2 = m2.add_constraint(isl.Constraint.ineq_from_names(space, {'x2': 1, 'n2': -1}))
bs2 = isl.BasicSet.universe(space.range())
bs2 = bs2.add_constraint(isl.Constraint.eq_from_names(space.range(), {'r2': 1, 'n2': -1}))
bs2 = bs2.add_constraint(isl.Constraint.ineq_from_names(space.range(), {'x2': 1, 'n2': -1}))

print '  ', m
print 'U ', m2
m_union = m.union(m2)
print '=>', m_union
m_closure, exact = m_union.transitive_closure()
print 'Closure:', m_closure, ' exact =',exact
m_closure = m_closure.coalesce().detect_equalities().remove_redundancies()
print 'Closure:', m_closure, ' exact =',exact
# apply to all non-recursive cases
m_result = m2.range().apply(m_closure)
print 'Result: ', m_result

print '\n\n'
m_closure, exact = m.transitive_closure()
print 'Closure:', m_closure, ' exact =',exact
m_closure = m_closure.coalesce().detect_equalities().remove_redundancies()
print 'Closure:', m_closure, ' exact =',exact
m_result = bs2.apply(m_closure).union(bs2).coalesce()
print 'Result: ', m_result


'''
[x, n, r]     -> [x2, n2, r2] : n2 > x2
[x, n, r]     -> [x2, n2, r2 = n2] : n2 <= x2
[x, n, r]     -> [x2, n2 = n, r2 = r] : x2 < n and x2 < x
[x, n, r = n] -> [x2, n2 = n, r2 = n] : n <= x2 < x
'''


print '\n\n'
space = isl.Space.create_from_names(ctx, set=['a', 'b', 'c'])
bset1 = isl.BasicSet.universe(space)
bset1 = bset1.add_constraint(isl.Constraint.eq_from_names(space, {'a': 1, 1: -1}))
space = isl.Space.create_from_names(ctx, set=['c', 'b', 'a'])
bset2 = isl.BasicSet.universe(space)
bset2 = bset2.add_constraint(isl.Constraint.eq_from_names(space, {'a': 1, 1: -3}))
print bset1
print bset2
print bset1.union(bset2)





print '\n\n'
space = isl.Space.create_from_names(ctx, set=['a', 'b', 'c'])
bset = isl.BasicSet.universe(space)
bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {'a': -1, 'b': 1, 1: -1})) # a < b
bset = bset.add_constraint(isl.Constraint.ineq_from_names(space, {'b': -1, 'c': 1, 1: -1})) # a < b
print bset
bset = bset.project_out(isl.dim_type.set, 1, 1)
print bset
print bset.find_dim_by_name(isl.dim_type.set, 'd')
print bset.find_dim_by_name(isl.dim_type.set, 'a')



# Test 'project_out' <== ? ==> 'make existential'
print '\n\n'
a = isl.BasicSet.read_from_str(ctx, '{[a, b, c] : a = b, b = c}')
print a
print a.project_out(isl.dim_type.set, 0, 1)
print a.project_out(isl.dim_type.set, 1, 1)
print a.project_out(isl.dim_type.set, 2, 1)
