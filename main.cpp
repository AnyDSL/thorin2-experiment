#include "thorin/world.h"

using namespace thorin;

int main()  {
    World w;
    auto Nat = w.assume(w.star(), "Nat");

    // λT:*.λx:T.x
    auto poly_id = w.lambda(w.star(), w.lambda(w.var(w.star(), 1), w.var(w.var(w.star(), 2), 1)));
    poly_id->dump();
    poly_id->type()->dump();

    // λx:Nat.x
    auto int_id = w.app(poly_id, Nat);
    int_id->dump();

    // 42
    w.app(int_id, w.assume(Nat, "42"))->dump();
}
