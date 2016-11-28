#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;

#define printValue(x) do{ printUtf8(x->name() == "" ? #x : x->name()); printUtf8(" = "); x->dump(); }while(0)
#define printType(x) do{ printUtf8(x->name() == "" ? #x : x->name()); printUtf8(": "); x->type()->dump(); }while(0)
#define printValueType(x) do{ printUtf8(x->name() == "" ? #x : x->name()); printUtf8(" = "); printUtf8(x->to_string()); printUtf8(": "); x->type()->dump(); }while(0)

void testNominal() {
    World w;
    auto Nat = w.nat();
    auto star = w.star();
    auto n42 = w.assume(Nat, "42");
    auto list = w.pi_lambda(w.pi(star, star), "list");
    auto cons = w.sigma_type(2, "cons");
    cons->set(0, w.var(star, 0));
    cons->set(1, w.app(list, w.var(star, 1)));
    assert(cons->is_closed());
    printValueType(cons);
    auto nil = w.sigma_type(0, "nil");
    assert(nil->is_closed());
    printValueType(nil);
    auto list_or_nil = w.variant({cons, nil});
    list->set(list_or_nil);
    assert(list->is_closed());
    printValueType(list);
    auto app = w.app(list, Nat);
    printValueType(app);

    cout << "--- NominalTest end ---" << endl;
}
