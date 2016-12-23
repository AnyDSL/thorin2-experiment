Constraint Checking
===================

This project is able to track the state of integer (`Nat`) variables and check if they fulfill given constraints. 
Constraints are given using the annotations `@guarantees` and `@accepts`. 
 
Example
-------
```
@guarantees "v3 = v1 + v2"
assume opNatPlus : (Nat, Nat) -> Nat;   // integer addition (v1, v2) -> v3

@accepts "0 <= v1 < 10"
assume get_finger : Nat -> *;           // v1 -> []
```
This program assumes two entities. For the integer addition, we _know_ that the result is the sum of the inputs - this is guaranteed. 
For the `get_finger` method, we can't say anything about it, but we know it can only handle inputs [0..9]. So we say it only _accepts_ the input parameter if `0 <= input < 10` holds. 

```
define test = get_finger (opNatPlus (5, 3));
```
If we apply the constraint checker to the program above, it will be able to show that this is (always) `[VALID]`.

```
define test2 = lambda n:Nat. get_finger (opNatPlus (5, n));
```
This statement is neither valid nor invalid - it depends on the given input parameter. The constraint checker reports this as `[PARTIAL] valid only for specific input`. 
Further it states: 
```
Variables:   [['p_1'], []]   // Function whose Nat input is named p_1, no Nat output 
Valid if:    { [p_1] : -5 <= p_1 <= 4 }
Invalid if:  { [p_1] : p_1 <= -6 or p_1 >= 5 }
```
We can see that the statement is valid only if the input is in [-5, 4], and that the statement is invalid otherwise. 


Basic Idea - variable tracking
------------------------------
Every `Nat` in a program gets its own, unique **variable**. These variables are contained in a **Set** (from isl library), including constraints between them. 

Each AST node gets a _structural variable list_ containing the variable names, structured similar to the node's type. 
Next, it gets a set tracking all _possible_ variable configurations, and a set tracking all _accepted_ variable configurations. 

**Examples:**
```
@guarantees "v3 = v1 + v2"
assume opNatPlus : (Nat, Nat) -> Nat;
// Variables =     [[v1, v2]   ,  v3]
// Accepted  = { [v1, v2, v3] : }               (no conditions)
// Possible  = { [v1, v2, v3] : v3 = v1 + v2 }  (addition is always correct)

@accepts "0 <= v1 < 10"
assume get_finger : Nat -> *;
// Variables =      [v1 , []]
// Accepted  = { [v1] : 0 <= v1 < 10 } (we want the input to be [0,9])
// Possible  = { [v1] : }              (we can't say anything about an input here)
```

When using these assumptions in other expressions, we can continue tracking the _accepted_ and _possible_ configurations. 
For the test2 statement, we will end up with something like this (simplified):
```
define test2 = lambda n:Nat. get_finger (opNatPlus (5, n));
// Variables = [n, []]
// Accepted  = { [n, v1_f, v1_a, v2_a, v3_a] : 0 <= v1_f < 10 }
// Possible  = { [n, v1_f, v1_a, v2_a, v3_a] : v1_a = 5 and v2_a = n and v1_f = v3_a }
```
You can see that `v1_f` (input of `get_finger`) is always equal to the output of the addition `v3_a`, whose inputs `v1_a, v2_a` are equal to `5, n`. 

To see if a statement is valid we check: 
"_Is every possible configuration also a valid one?_"
which is equal to "_Is `possible` a subset of `accepted`?_"
This is clearly not the case in our example, so this expression is not always valid.  

To see when this expression is invalid, we calculate the _error set_: 
`error_set = possible - accepted`. Now we can see the possible, but invalid configurations. 
We can also get the set of all valid configurations: `valid_set = possible intersected with accepted = (invert error_set) intersected with possible`. 
```
// error_set = { [n, v1_f, v1_a, v2_a, v3_a] : v1_a = 5 and v2_a = n and v1_f = v3_a and (v1_f < 0 or v1_f >= 10) }
// valid_set = { [n, v1_f, v1_a, v2_a, v3_a] : v1_a = 5 and v2_a = n and v1_f = v3_a and 0 <= v1_f < 10 }
```
If `valid_set` is empty, we know that this statement is always invalid. 

To simplify the error condition, we can make all _bound_ variables existential, and simplify the set: 
```
// error_set = { [n, v1_f, v1_a, v2_a, v3_a] : v1_a = 5 and v2_a = n and v1_f = v3_a and (v1_f < 0 or v1_f >= 10) }
// => { [n] : exists [v1_f, v1_a, v2_a, v3_a] with v1_a = 5 and v2_a = n and v1_f = v3_a and 0 <= v1_f < 10 }
// => { [n] : -5 <= n <= 9}
```
This gives us nice, human-readable error conditions (and additional constraints, if we invert this new set).


Implementation
--------------
TODO


Recursive functions
-------------------
Recursive functions are a special case, as their constraints can't be derived automatically. 
There is a heuristic deriving (possibly valid) input/output constraints, but it is likely to fail on complex examples. 
In this case, you have to annotate recursive functions by hand:
```
define test = lambda n:Nat.
 	@accepts "0 <= x < n"
 	@guarantees "f1 > x and f1 > n"
	lambda rec f(x:Nat):Nat.  f(x+n);
```
Checking is a two-step process in this case. First, all recursive functions are checked if they fulfill their annotated constraints. 
If they do, the outer code is checked for validity. 


Advantages / Disadvantages
--------------------------
This approach is quite powerful in tracking variable configurations, but it has one big drawback: dead code / empty set. 
In some cases (for example constrained callbacks), it can happen that (partially) dead code is analyzed, and generates invalid constraints. 
```
// for (i = 0; i < 0; i++): result += i;
// return (result, ...);
define test = (sum 0 (lambda i: toFloat i) , ...);
```
In this case, `sum` generates a constraint in `possible` looking like `0 <= i < n` and `n = 0`, which is unsolvable. 
It follows that the `possible` set is empty, which makes the condition `possible subset of accepted` always true - no matter what `...` is.   
I believe this issue can be mitigated by careful constraint generation - but this is left to _future work_ (and other developers). 

