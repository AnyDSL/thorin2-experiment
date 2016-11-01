#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;


#define printValue(x) do{ printUtf8(#x " = "); x->dump(); }while(0)
#define printType(x) do{ printUtf8(#x ": "); x->type()->dump(); }while(0)



static void testCompiled(World& w);
static void testRecursive(World& w);
static void testLUDecomposition(World& w);


void testMatrix() {
	World w;
	auto Nat = w.nat();
	auto Float = w.assume(w.star(), "Float");
	auto opFloatPlus = w.assume(w.pi(w.sigma({ Float, Float }), Float), "+");
	auto opFloatMult = w.assume(w.pi(w.sigma({ Float, Float }), Float), "x");
	auto cFloatZero = w.assume(Float, "0.0");

	// Some testing
	auto getSecondValue = w.lambda({ Nat, Nat }, w.extract(w.var({ Nat, Nat }, 1), 1));
	printValue(getSecondValue);
	printType(getSecondValue);
	
	
	auto ArrTypeFuncT = w.pi(Nat, w.star());
	printValue(ArrTypeFuncT);
	// Type constructor for arrays
	auto ArrT = w.assume(w.pi({Nat , ArrTypeFuncT }, w.star()), "ArrT");
	printType(ArrT);

	// Array Creator
	// (n: Nat, tf: ArrTypeFuncT) -> (pi i:Nat -> tf i) -> (ArrT (n, tf))
	auto arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 2));
	auto arrcreatorfunctype = w.pi(Nat, w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 2), 1), w.var(Nat, 1)));
	auto ArrCreate = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrcreatorfunctype, arrtype)), "ArrCreate");
	printValue(ArrCreate);
	printType(ArrCreate);

	// Array Getter
	// (n: Nat, tf: ArrTypeFuncT) -> (ArrT (n, tf)) -> (pi i:Nat -> tf i)
	arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 1));
	auto resulttype = w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 3), 1), w.var(Nat, 1));
	auto ArrGet = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrtype, w.pi(Nat, resulttype))), "ArrGet");
	printValue(ArrGet);
	printType(ArrGet);

	// Matrix Type
	//auto MatrixType = w.lambda()

	// MapReduce
	auto t = [&](int i) {return w.var(w.star(), i); };
	auto reduce = w.assume(w.pi(w.star(), //t
		w.pi(w.pi(w.sigma({ t(1), t(1) }), t(2)), // (t x t -> t)
				w.pi(t(2), w.pi(Nat, // t startvalue, Nat len
					w.pi(w.pi(Nat, t(5)), // (Nat -> t)
						t(5)
					)
				))
			)
	), "reduce");
	printType(reduce);
	auto sum = w.app(w.app(w.app(reduce, Float), opFloatPlus), cFloatZero, "sum");
	printType(sum);



	printUtf8("\n--- compiled ---\n\n");
	testCompiled(w);

	printUtf8("\n--- recursive ---\n\n");
	testRecursive(w);

	printUtf8("\n--- LU Decomposition ---\n\n");
	testLUDecomposition(w);

	printUtf8("\n--- end ---\n\n");
}


void testCompiled(World& w) {
#include "frontend/arrays.lbl.h"
}


void testRecursive(World& w) {
	auto Nat = w.nat();
	auto opNatPlus = w.assume(w.pi({Nat, Nat}, Nat), "+");
	auto cNatOne = w.assume(Nat, "1n");
	auto addToInfinity = w.lambdaRec(Nat, Nat, "addToInfinity");
	printValue(addToInfinity);
	printType(addToInfinity);
	addToInfinity->setBody(w.app(opNatPlus, {w.app(addToInfinity, w.var(Nat, 0)), cNatOne}));
	printValue(addToInfinity);
	printValue(addToInfinity->body());
	printType(addToInfinity);
}


void testLUDecomposition(World& w) {
#include "frontend/lu-decomposition.lbl.h"
}


