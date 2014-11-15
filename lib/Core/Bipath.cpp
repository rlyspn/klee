/**
 * TODO(Riley Spahn): License
 */

#include "Bipath.h"

#include <stdio.h>
#include "klee/Internal/Module/KInstruction.h"
#include "llvm/Function.h"
#include "Memory.h"  // for MemoryObject

using namespace klee;
using namespace llvm;

Bipath::Bipath() {
    /* TODO */
}

Bipath::~Bipath() {
    /* TODO */
}

bool Bipath::isEvaluated(ExecutionState &state, KInstruction *ki,
    llvm::Function *f, std::vector< ref<Expr> > &arguments) {

    printf("Call %s() with %lu args:",
        f->getName().data(), arguments.size());
    for (unsigned i = 0; i < arguments.size(); i++) {
        printf(" %08x", arguments[i]->hash());
    }
    printf("\n");

    printf("\t\tSymbolic variables:");
    for (unsigned i = 0; i < state.symbolics.size(); i ++) {
        printf(" [%s]", state.symbolics[i].first->name.c_str());
    }
    printf("\n");

    return false;
}
