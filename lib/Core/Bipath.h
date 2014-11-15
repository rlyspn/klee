/**
 * TODO(Riley Spahn): License
 */

#ifndef BIPATH_H
#define BIPATH_H

#include <vector>
#include "klee/ExecutionState.h"

namespace llvm {
    class Function;
    class KInstruction;
}

namespace klee {

class Executor;
class Expr;
template<class T> class ref;

class Bipath {

public:
    Bipath();
    ~Bipath();

    /**
     * Tests if the function with the given arguments have been executed
     * symbolically.
     *
     * \param f The function to test.
     * \param arguments The arguments passed to the function.
     */
    bool isEvaluated(Executor *executor, ExecutionState &state,
        KInstruction *ki, llvm::Function *f,
        std::vector< ref<Expr> > &arguments);

    void makeSymbolic(Executor *executor, ExecutionState &state,
        KInstruction *target, ref<Expr> expr,
        const std::string &argument_name, size_t argument_size);
};

}

#endif // BIPATH_H
