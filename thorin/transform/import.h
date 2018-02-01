#ifndef THORIN_TRANSFORM_IMPORT_H
#define THORIN_TRANSFORM_IMPORT_H

#include "thorin/world.h"

namespace thorin {

class Importer {
public:
    Importer(World& src)
        : world_(src.name())
    {
        //if  (src.is_pe_done())
            //world_.mark_pe_done();
#ifndef NDEBUG
        if (src.track_history())
            world_.enable_history(true);
#endif
    }

    World& world() { return world_; }
    const Def* import(Tracker);
    bool todo() const { return todo_; }

public:
    void check_todo(const Def*, const Def*);

    Def2Def old2new_;
    World world_;
    bool todo_ = false;
};

}

#endif
