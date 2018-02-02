#include "thorin/core/world.h"
#include "thorin/util/stream.h"

#include <iostream>
#include <sstream>

namespace thorin::core {

void emit() {
    World world;
}

}

int main(int, const char**) {
    thorin::core::emit();
    return EXIT_SUCCESS;
}
