/**
 * TODO(Riley Spahn): License
 */

#ifndef BIPATH_H
#define BITPATH_H

#include <vector>

namespace llvm {
    class Function;
}

namespace klee {

class Expr;
template<class T> class ref;

class Bipath {
    Bipath();
    ~Bipath();

    /**
     * Tests if the function with the given arguments have been executed
     * symbolically.
     *
     * \param f The function to test.
     * \param arguments The arguments passed to the function.
     */
    bool isEvaluated(const llvm::Function *f, const std::vector< ref<Expr> > &arguments);
};

}

#endif // BIPATH_H
