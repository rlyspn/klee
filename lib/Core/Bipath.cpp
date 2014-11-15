/**
 * TODO(Riley Spahn): License
 */

#include "Bipath.h"

#include <stdio.h>
#include <sstream>
#include <climits>

#include "llvm/Function.h"
#include "llvm/Argument.h"
#include "llvm/Type.h"

#include "klee/Internal/Module/KInstruction.h"
#include "Memory.h"  // for MemoryObject
#include "Executor.h"
#include "TimingSolver.h"

using namespace klee;
using namespace llvm;

Bipath::Bipath() {
    /* TODO */
}

Bipath::~Bipath() {
    /* TODO */
}

bool Bipath::isEvaluated(Executor *executor, ExecutionState &state,
    KInstruction *ki, llvm::Function *f,
    std::vector< ref<Expr> > &arguments) {

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

    if(f->getName().str() == "max") {
        static unsigned count = 0;
        if(count < arguments.size()) {
            //std::ostringstream name;
            //name << "arg_" << count;
            //makeSymbolic(state, ki, ADDRESS, name.str(), SIZE);

            if(arguments[count]->getKind() == Expr::Constant) {
                ConstantExpr *c = static_cast<ConstantExpr *>(
                    &*arguments[count]);

                std::string arg;
                c->toString(arg);
                printf("ARGUMENT %d is a constant expression: [%s]\n",
                    count, arg.c_str());
            }

            llvm::Function::arg_iterator it = f->arg_begin();
            for(unsigned z = 0; z < count; z ++) it++;

            printf("ARGUMENT %d name is [%s]\n",
                count, (*it).getName().str().c_str());

            llvm::Argument *argument = &(*it);
            size_t argument_size
                = argument->getType()->getScalarSizeInBits() / CHAR_BIT;
            std::string argument_name = argument->getName().str();

            makeSymbolic(executor, state, ki,
                arguments[count], argument_name, argument_size);

            count ++;
        }
    }

    return false;
}

#if 1
// NOTE: this function is cloned from
// SpecialFunctionHandler::handleMakeSymbolic()
void Bipath::makeSymbolic(Executor *executor, ExecutionState &state,
    KInstruction *target, ref<Expr> expr, const std::string &argument_name,
    size_t argument_size) {

    ref<Expr> size_expr = ConstantExpr::alloc(
        argument_size, Context::get().getPointerWidth());

    // look for all variables aliased to the given address
    Executor::ExactResolutionList rl;
    executor->resolveExact(state, expr, rl, "bipath_make_symbolic");

    for (Executor::ExactResolutionList::iterator it = rl.begin(), 
        ie = rl.end(); it != ie; ++it) {

        const MemoryObject *mo = it->first.first;
        mo->setName(argument_name);  // set name to arg_*

        const ObjectState *old = it->first.second;
        ExecutionState *s = it->second;

        if (old->readOnly) {
            executor->terminateStateOnError(*s, 
                "cannot make readonly object symbolic", 
                "user.err");
            return;
        } 

        // FIXME: Type coercion should be done consistently somewhere.
        bool res;
        bool success =
            executor->solver->mustBeTrue(*s, 
                EqExpr::create(ZExtExpr::create(size_expr,
                    Context::get().getPointerWidth()),
                  mo->getSizeExpr()),
                res);
        assert(success && "FIXME: Unhandled solver failure");

        if (res) {
            executor->executeMakeSymbolic(*s, mo, argument_name);
        } else {      
            executor->terminateStateOnError(*s, 
                "wrong size given to klee_make_symbolic[_name]", 
                "user.err");
        }
    }
}
#endif
