#ifndef COC_VISITOR_H
#define COC_VISITOR_H

#include "thorin/def.h"

namespace thorin {

class Visitor {
public:
    virtual void visit(const Unbound* e) = 0;
    virtual void visit(const App* e) = 0;
    virtual void visit(const Star* e) = 0;
    virtual void visit(const Assume* e) = 0;
    virtual void visit(const Var* e) = 0;
    virtual void visit(const Sigma* e) = 0;
    virtual void visit(const Tuple* e) = 0;
    virtual void visit(const Pi* e) = 0;
    virtual void visit(const Lambda* e) = 0;
};

}

#endif //COC_VISITOR_H
