#include "thorin/transform/import.h"

namespace thorin {

const Def* Importer::import(Tracker old_def) {
    if (auto new_def = find(old2new_, old_def)) {
        assert(&new_def->world() == &world_);
        assert(!new_def->is_replaced());
        return new_def;
    }

    auto new_type = import(old_def->type());
    Def* new_nominal = nullptr;
    if (old_def->is_nominal())
        new_nominal = insert(old_def, old_def->stub(world(), new_type));

    DefArray new_ops(old_def->num_ops());
    for (size_t i = 0, e = old_def->num_ops(); i != e; ++i) {
        new_ops[i] = import(old_def->op(i));
        todo_ |= new_ops[i]->tag() != old_def->tag();
    }

    if (new_nominal != nullptr) {
        for (size_t i = 0, e = new_ops.size(); i != e; ++i)
            new_nominal->set(i, new_ops[i]);
        return new_nominal;
    }

    auto new_def = old_def->rebuild(world(), new_type, new_ops);
    if (old_def->num_ops() == new_def->num_ops()) {
        for (size_t i = 0, e = old_def->num_ops(); !todo_ && i != e; ++i)
            todo_ |= old2new_[old_def->op(i)] == new_def->op(i);
    } else {
        todo_ = true;
    }

    return insert(old_def, new_def);
}

}
