Lambda Calculus Frontend
========================

This project provides a Python 2 frontend to CoC lambda expressions. 

Features
--------
- Read lambda expressions from a simple text format
- Convert them to C++ code, that generates the lambda expressions in CoC
- A (partial) Python implementation of CoC's type-checking process
- Constraint checking on natural numbers. 
This analysis can guarantee (for example) that an array access is never out of bounds.


Setup
-----
### Python setup
This project requires Python 2 and several modules. Installing is simple using `pip`. 
You can install the needed modules globally, or in a [virtualenv](http://docs.python-guide.org/en/latest/dev/virtualenvs/):
```
pip install pypeg2 islpy
```

### (optional) compile islpy
To speed up constraint checking, you can build our extended version of `islpy`: [TODO](TODO). 
You get the best results using Clang+LTO, but GCC will also work. 

- Checkout the extended `islpy`
- Build the extension. Use one of
  - `./clang-build.sh` for Clang+LTO
  - `python configure.py --cxxflags "-O3" --ldflags "-O3" && python setup.py build` for GCC (the original islpy library does not even apply compiler optimization)
- Install the compiled extension, or just symlink it: `ln -s <islpy-root>/build/lib.<???>/islpy <frontend-root>/islpy`
- If it worked, the frontend will greet you with `[islpy] Native extension present`. 


Basics
------
Lambda expressions are given as _Assumptions_ and _Definitions_. 
Assumptions are declared using their type. Example:

```
assume opNatPlus: (Nat,Nat)->Nat;        // Assume this function exists
define inc = λ x:Nat. opNatPlus (x,1);   // Define using previous assumption
define two = inc 1;
```

You can run the example script over this input:
`python frontend.py <input.lbl>`

To see the API of this project, look into [frontend.py](frontend.py). 


Lambda files
------------
Lambda files have the extension `.lbl` and must be encoded as UTF-8. 
Currently, there is no specification for lambda files. 
Just refer to the given examples, they use all features (including all possible annotations).
You can always write `lambda, pi, sigma` instead of `λ, Π, Σ`.
 
- [stdlib.lbl](programs/stdlib.lbl),  some basic assumptions
- [arrays.lbl](programs/arrays.lbl),  a formalization of arrays
- [matrixmultiplication.lbl](programs/matrixmultiplication.lbl),  a simple program


Constraint checking
-------------------
Constraint checking is based on the isl library. It is [explained here](constraint_checking.md).



Code Structure
--------------
TODO
  