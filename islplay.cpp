//
// Created by markus on 06.11.16.
//

#include "utils/unicodemanager.h"
#include "thorin/world.h"
#include "thorin/visitor.h"

#include <isl/ctx.h>
#include <isl/printer.h>
#include <isl/set.h>
#include <isl/space.h>

#define printValue(x)
#define printType(x)



using namespace thorin;

class TestVisitor : public Visitor {
private:
    World& w;
    const Def* ArrGet;

public:

    TestVisitor(World &w, const Def *ArrGet) : w(w), ArrGet(ArrGet) {}

    virtual void visit(const App* e) override {
        if (e->callee() == ArrGet){
            //TODO check bounds
            printUtf8("Reached ArrGet call\n");
        }else{
            // check callee
            e->callee()->accept(*this);
        }
        // check args
        for (auto arg: e->args()){
            arg->accept(*this);
        }
    }

    virtual void visit(const Assume* e) override {
        if (e == ArrGet){
            printUtf8("[WARN] Reached ArrGet without application - can't check further usage\n");
        }
    }

    virtual void visit(const Lambda* e) override {
        if (e->is_nominal()){
            //TODO
        }else{
            e->body()->accept(*this);
        }
    }

    virtual void visit(const Pi* e) override {
        if (e->is_nominal()){
            //TODO
        }else{
            e->body()->accept(*this);
        }
    }

    virtual void visit(const Var* e) override {

    }

    virtual void visit(const Sigma* e) override {
        for (auto op: e->ops())
            op->accept(*this);
    }

    virtual void visit(const Tuple* e) override {
        for (auto op: e->ops())
            op->accept(*this);
    }

    virtual void visit(const Star* e) override {

    }

    virtual void visit(const Unbound* e) override {

    }

};




void testISL(){
    World w;
#include "frontend/arrays-test.lbl.h"

    TestVisitor v(w, ArrGet);
    // typecheck arrayAccess
    arrayAccess->accept(v);
    printUtf8("Analysis finished.\n");



    printUtf8("I'm alive\n");

    isl_ctx *ctx = isl_ctx_alloc();
    isl_printer *printer = isl_printer_to_str(ctx);
    isl_printer_set_output_format(printer , ISL_FORMAT_ISL);

    // input n
    // arr a = new arr[n]
    // for (i = 0; i < n; i++)
    // access a[i] -> check 0 <= i < n

    isl_space* dim = isl_space_set_alloc(ctx, 0, 0);
    isl_basic_set * s1 = isl_basic_set_universe(isl_space_copy(dim));
    isl_printer_print_basic_set(printer, s1);
    isl_basic_set_free(s1);
    isl_space_free(dim);


    char *output = isl_printer_get_str(printer);
    isl_printer_flush(printer);
    printUtf8(output);
    printUtf8("\n");
    free(output);

    isl_printer_free(printer);
    isl_ctx_free(ctx);

    printUtf8("I'm done.\n");
}
