/**
 * TODO(Riley Spahn): License
 */

#include "Bipath.h"

using namespace klee;
using namespace llvm;

Bipath::Bipath() {
    /* TODO */
}

Bipath::~Bipath() {
    /* TODO */
}

bool Bipath::isEvaluated(const llvm::Function *f, const std::vector< ref<Expr> > &arguments) {
    return false;
}
