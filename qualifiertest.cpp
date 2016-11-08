#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;

void testQualifiers() {
    auto U = Qualifier::Unrestricted;
    auto R = Qualifier::Relevant;
    auto A = Qualifier::Affine;
    auto L = Qualifier::Linear;

    assert(A < U);
    assert(R < U);
    assert(L < U);
    assert(L < A);
    assert(L < R);

    auto meet = [](Qualifier::URAL a, Qualifier::URAL b) { return Qualifier::meet(a, b); };
    assert(meet(U, U) == U);
    assert(meet(A, U) == A);
    assert(meet(R, U) == R);
    assert(meet(L, U) == L);
    assert(meet(A, A) == A);
    assert(meet(A, R) == L);
    assert(meet(L, A) == L);
    assert(meet(L, R) == L);

    World w;
    auto Nat = w.nat();
    auto ANat = w.nat(A);
    auto RNat = w.nat(R);
    auto LNat = w.nat(L);
    auto an0 = w.assume(ANat, "0");
    auto anx = w.var(ANat, 1, "x");
    auto anid = w.lambda(ANat, anx, "anid");
    auto app1 = w.app(anid, an0);
    assert(w.app(anid, an0) == w.error());

    auto tuple_type = w.sigma({ANat, RNat});
    assert(tuple_type->qualifier() == L);
    auto an1 = w.assume(ANat, "1");
    auto rn0 = w.assume(RNat, "0");
    auto tuple = w.tuple({an1, rn0});
    assert(tuple->type() == tuple_type);
    auto tuple_app0 = w.extract(tuple, 0);
    assert(w.extract(tuple, 0) == w.error());

    auto a_id_type = w.pi(Nat, Nat, A);
    auto nx = w.var(Nat, 1, "x");
    auto a_id = w.lambda(Nat, nx, A, "a_id");
    assert(a_id_type == a_id->type());
    auto n0 = w.assume(Nat, "0");
    auto a_id_app = w.app(a_id, n0);
    assert(w.app(a_id, n0) == w.error());

    // λᴬT:*.λx:ᴬT.x
    auto aT = w.var(w.star(A), 2, "T");
    auto x = w.var(aT, 1, "x");
    auto poly_aid = w.lambda(aT->type(), w.lambda(aT, x));
    poly_aid->dump();
    poly_aid->type()->dump();

    // λx:ᴬNat.x
    auto anid2 = w.app(poly_aid, ANat);
    anid2->dump();
    anid2->type()->dump();
}
