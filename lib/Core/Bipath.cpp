/**
 * TODO(Riley Spahn): License
 */

#include "Bipath.h"

#include <stdio.h>

using namespace klee;
using namespace llvm;

Bipath::Bipath() {
    /* TODO */
}

Bipath::~Bipath() {
    /* TODO */
}

bool Bipath::isEvaluated(llvm::Function *f, std::vector< ref<Expr> > &arguments) {
    return false;
}
