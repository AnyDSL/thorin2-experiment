#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;


#define printValue(x) do{ printUtf8(#x " = "); x->dump(); }while(0)
#define printType(x) do{ printUtf8(#x ": "); x->type()->dump(); }while(0)



void testMatrix() {
	World w;
	auto Nat = w.nat();
	auto Float = w.assume(w.star(), "Float");
	auto opFloatPlus = w.assume(w.pi(w.sigma({ Float, Float }), Float), "+");
	auto opFloatMult = w.assume(w.pi(w.sigma({ Float, Float }), Float), "x");
	auto cFloatZero = w.assume(Float, "0.0");

	// Some testing
	auto getSecondValue = w.lambda({ Nat, Nat }, w.extract(w.var({ Nat, Nat }, 0), 1));
	printValue(getSecondValue);
	printType(getSecondValue);


	auto ArrTypeFuncT = w.pi(Nat, w.star());
	printValue(ArrTypeFuncT);
	// Type constructor for arrays
	auto ArrT = w.assume(w.pi({Nat , ArrTypeFuncT }, w.star()), "ArrT");
	printType(ArrT);

	// Array Creator
	// (n: Nat, tf: ArrTypeFuncT) -> (pi i:Nat -> tf i) -> (ArrT (n, tf))
	auto arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 1));
	auto arrcreatorfunctype = w.pi(Nat, w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 1), 1), w.var(Nat, 0)));
	auto ArrCreate = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrcreatorfunctype, arrtype)), "ArrCreate");
	printValue(ArrCreate);
	printType(ArrCreate);

	// Array Getter
	// (n: Nat, tf: ArrTypeFuncT) -> (ArrT (n, tf)) -> (pi i:Nat -> tf i)
	arrtype = w.app(ArrT, w.var({ Nat , ArrTypeFuncT }, 0));
	auto resulttype = w.app(w.extract(w.var({ Nat , ArrTypeFuncT }, 2), 1), w.var(Nat, 0));
	auto ArrGet = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.pi(arrtype, w.pi(Nat, resulttype))), "ArrGet");
	printValue(ArrGet);
	printType(ArrGet);

	// Matrix Type
	//auto MatrixType = w.lambda()

	// MapReduce
	auto t = [&](int i) {return w.var(w.star(), i); };
	auto reduce = w.assume(w.pi(w.star(), //t
		w.pi(w.pi(w.sigma({ t(0), t(0) }), t(1)), // (t x t -> t)
				w.pi(t(1), w.pi(Nat, // t startvalue, Nat len
					w.pi(w.pi(Nat, t(4)), // (Nat -> t)
						t(4)
					)
				))
			)
	), "reduce");
	printType(reduce);
	auto sum = w.app(w.app(w.app(reduce, Float), opFloatPlus), cFloatZero, "sum");
	printType(sum);



	printUtf8("\n--- compiled ---\n\n");



	// UArrT := λ lt:(Nat, *). ArrT (lt[0], λ _:Nat. lt[1])
	auto UArrT = w.lambda({ Nat, w.star() }, w.app(ArrT, { w.extract(w.var({ Nat, w.star() }, 0, "lt"), 0), w.lambda(Nat, w.extract(w.var({ Nat, w.star() }, 1, "lt"), 1)) }), "UArrT");
	printValue(UArrT);
	printType(UArrT);

	// MatrixType := λ n:Nat. λ m:Nat. UArrT (n, UArrT (m, Float))
	auto MatrixType = w.lambda(Nat, w.lambda(Nat, w.app(UArrT, { w.var(Nat, 1, "n"), w.app(UArrT,{ w.var(Nat, 0, "m"), Float }) })));
	printValue(MatrixType);
	printType(MatrixType);

	// matrixDot := λ n:Nat. λ m:Nat. λ o:Nat. λ M1:MatrixType n m. λ M2:MatrixType m o. ArrCreate (n, λ _:Nat. UArrT (o, Float)) (λ i:Nat. ArrCreate (o, λ _:Nat. Float) (λ j:Nat. sum n (λ k:Nat. opFloatMult ((ArrGet (m, λ _:Nat. Float) (ArrGet (n, λ _:Nat. UArrT (m, Float)) M1 i) k), (ArrGet (o, λ _:Nat. Float) (ArrGet (m, λ _:Nat. UArrT (m, Float)) M1 k) j)))))
	auto matrixDot = w.lambda(Nat, w.lambda(Nat, w.lambda(Nat, w.lambda(w.app(w.app(MatrixType, w.var(Nat, 2, "n")), w.var(Nat, 1, "m")), w.lambda(w.app(w.app(MatrixType, w.var(Nat, 2, "m")), w.var(Nat, 1, "o")), w.app(w.app(ArrCreate, {w.var(Nat, 4, "n"), w.lambda(Nat, w.app(UArrT, {w.var(Nat, 3, "o"), Float}))}), w.lambda(Nat, w.app(w.app(ArrCreate, {w.var(Nat, 3, "o"), w.lambda(Nat, Float)}), w.lambda(Nat, w.app(w.app(sum, w.var(Nat, 6, "n")), w.lambda(Nat, w.app(opFloatMult, {w.app(w.app(w.app(ArrGet, {w.var(Nat, 6, "m"), w.lambda(Nat, Float)}), w.app(w.app(w.app(ArrGet, {w.var(Nat, 7, "n"), w.lambda(Nat, w.app(UArrT, {w.var(Nat, 7, "m"), Float}))}), w.var(w.app(w.app(MatrixType, w.var(Nat, 7, "n")), w.var(Nat, 6, "m")), 5, "M1")), w.var(Nat, 2, "i"))), w.var(Nat, 0, "k")), w.app(w.app(w.app(ArrGet, {w.var(Nat, 5, "o"), w.lambda(Nat, Float)}), w.app(w.app(w.app(ArrGet, {w.var(Nat, 6, "m"), w.lambda(Nat, w.app(UArrT, {w.var(Nat, 7, "m"), Float}))}), w.var(w.app(w.app(MatrixType, w.var(Nat, 7, "n")), w.var(Nat, 6, "m")), 5, "M1")), w.var(Nat, 0, "k"))), w.var(Nat, 1, "j"))}))))))))))));
	printValue(matrixDot);
	printType(matrixDot);



	printUtf8("\n--- end ---\n\n");
}


