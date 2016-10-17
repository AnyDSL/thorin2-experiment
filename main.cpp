#include "thorin/world.h"

using namespace thorin;

int main()  {
    World w;
    auto n1 = w.assume(w.nat(), "1");
    auto n2 = w.assume(w.nat(), "2");
    auto n3 = w.assume(w.nat(), "3");

    // λT:*.λx:T.x
    auto poly_id = w.lambda(w.star(), w.lambda(w.var(w.star(), 1), w.var(w.var(w.star(), 2), 1)));
    poly_id->dump();
    poly_id->type()->dump();

    // λx:w.nat().x
    auto int_id = w.app(poly_id, w.nat());
    int_id->dump();

    // 2
    w.app(int_id, n2)->dump();

    // 3
    w.extract(w.tuple({n1, n2, n3}), 2)->dump();
    w.extract(n3, 0)->dump();

    auto make_pair = w.assume(w.pi(w.unit(), w.sigma({w.nat(), w.nat()})), "make_pair");
    w.app(make_pair, n1)->dump();

    auto plus = w.assume(w.pi({w.nat(), w.nat()}, w.nat()), "+");
    w.app(int_id, w.app(plus, {w.app(plus, {n1, n2}), n3}))->dump();

    auto Arr = w.assume(w.pi({w.nat(), w.pi(w.nat(), w.star())}, w.star()));
    //auto _Arr = w.lambda({w.nat(), w.star()}, w.app(Arr, {w.extract(w.var(
}
