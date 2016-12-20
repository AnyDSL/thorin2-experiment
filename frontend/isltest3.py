import islpy as isl

ctx = isl.DEFAULT_CONTEXT

bset1 = isl.BasicSet.read_from_str(ctx, '{[a, b] : a <= b}')
bset2 = isl.BasicSet.read_from_str(ctx, '{[a, c] : c <= a}')
print bset1
print bset2
print bset1.ext_join
print bset1.ext_join(bset2)
print bset1.ext_join(bset1)

