#include "thorin/world.h"

using namespace thorin;

// TODO remove this macro
#define print_value_type(x) do{ std::cout << (x->name() == "" ? #x : x->name()) << " <" << x->gid() << "> = " << x << ": " << x->type() << endl; }while(0)

void testNominal() {
    World w;
    auto Nat = w.nat();
    auto star = w.star();

    std::cout << "--- NominalTest begin ---" << endl;
    auto s1 = w.sigma_type(1, {"N"});
    auto v1 = w.var(star, 0);
    s1->set(0, v1);
    assert(s1->is_closed());
    assert(s1->has_free_var(0));
    assert(s1->has_free_var_in(0, 1));
    print_value_type(s1);
    auto l1 = w.lambda(star, w.sigma({s1, s1}));
    print_value_type(l1);
    auto a1 = w.app(l1, Nat);
    print_value_type(a1);
    a1->ops().dump();
    assert(a1->op(0) != a1->op(1));

    auto list = w.pi_lambda(w.pi(star, star), {"list"});
    auto cons = w.sigma_type(2, {"cons"});
    cons->set(0, w.var(star, 0));
    cons->set(1, w.app(list, w.var(star, 1)));
    assert(cons->is_closed());
    assert(cons->has_free_var_in(0, 1));
    print_value_type(cons);
    auto nil = w.sigma_type(0, {"nil"});
    assert(nil->is_closed());
    print_value_type(nil);
    auto list_or_nil = w.variant({cons, nil});
    list->set(list_or_nil);
    assert(list->is_closed());
    print_value_type(list);
    std::cout << "app list Nat" << endl;
    auto apped = w.app(list, Nat);
    print_value_type(apped);
    print_value_type(apped->op(0));
    print_value_type(apped->op(1));

    auto body_type = w.app(list, w.var(star, 0));
    auto pi_wrap = w.pi(star, body_type);
    print_value_type(pi_wrap);
    print_value_type(pi_wrap->op(1)->op(0));
    print_value_type(pi_wrap->op(1)->op(1));
    auto wrap = w.axiom(pi_wrap, {"axiom_wrap"});
    auto wrap_app = w.app(wrap, Nat);
    print_value_type(wrap_app);
    print_value_type(wrap_app->type()->op(0));
    print_value_type(wrap_app->type()->op(1));
    auto lam = w.pi_lambda(pi_wrap, w.any(body_type, w.tuple(nil, {})));
    print_value_type(lam);
    print_value_type(w.app(lam, Nat));
    cout << "--- NominalTest end ---" << endl;
}
