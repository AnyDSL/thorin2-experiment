#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;


#define printValue(x) do{ printUtf8(#x " = "); x->dump(); }while(0)
#define printType(x) do{ printUtf8(#x ": "); x->type()->dump(); }while(0)

void testMatrix() {
	World w;
	
	auto Nat = w.nat();
	auto ArrTypeFuncT = w.pi(Nat, w.star());
	printValue(ArrTypeFuncT);
	// Type constructor for arrays
	auto ArrT = w.assume(w.pi({Nat , ArrTypeFuncT }, w.star()), "ArrT");
	printType(ArrT);
	// Array Creator
	// (n: Nat, tf: ArrTypeFuncT) -> (pi i -> tf i) -> (ArrT (n, tf))
	//auto typefunc = w.extract(w.var({ Nat , ArrTypeFuncT }, 2), 1);
	//auto ArrCreate = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.pi(w.pi(Nat, , w.app(ArrT, w.var(Nat, 2))));
	auto ArrCreate = w.assume(w.pi({ Nat , ArrTypeFuncT }, w.app(ArrT, w.var(Nat, 1))), "ArrCreate");
	printValue(ArrCreate);
	printType(ArrCreate);


	printUtf8("\n--- end ---\n\n");
}

