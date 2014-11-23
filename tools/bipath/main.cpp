/***
 * TODO: License
 */

#include "expr/Parser.h"

#include "klee/ExprBuilder.h"
#include "klee/util/ExprPPrinter.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <dirent.h>
#include <sys/types.h>
#include <vector>

/* TODO: Kleaver had to do the same thing. */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "llvm/Support/system_error.h"

#define BIPATH_MAX_PARSER_ERROR 20

using namespace llvm;
using namespace klee;
using namespace klee::expr;

/* Returns all files with a .pcs file ending. */
static bool getPCFiles(const std::string inputDir,
        std::vector<std::string> *pcsFiles)
{
    DIR *dir;
    dirent *ent;

    if ((dir = opendir(inputDir.c_str())) == NULL) {
        llvm::errs() << "Could not read: " << inputDir << "\n";
        return false;
    }

    while ((ent = readdir(dir)) != NULL) {
        std::string file_name(ent->d_name);
        std::size_t index = file_name.find(".pc");
        if (index != std::string::npos && index == file_name.size() - 3) {
            pcsFiles->push_back(inputDir + file_name);
        }
    }
    closedir(dir);


    return true;
}

static ref<Expr> mergeConstraints(ref<Expr> constr1, ref<Expr> constr2)
{
    std::vector<Expr::CreateArg> args(2);
    Expr::CreateArg arg1(constr1);
    Expr::CreateArg arg2(constr2);
    args.push_back(arg1);
    args.push_back(arg2);
    errs() << "Combining: \n\t";
    ExprPPrinter::printSingleExpr(errs(), constr1);
    errs() << "\n\t";
    ExprPPrinter::printSingleExpr(errs(), constr2);
    errs() << "\n";
    errs() << args[0].isExpr() << "\n";
    errs() << args[1].isExpr() << "\n";
    errs() << args[0].isWidth() << "\n";
    errs() << args[1].isWidth() << "\n";


    return Expr::createFromKind(Expr::And, args);
}

static bool rewriteConstraints(const std::string inputFile)
{
    ExprBuilder *builder;
    OwningPtr<MemoryBuffer> mb;
    error_code error = MemoryBuffer::getFileOrSTDIN(inputFile.c_str(), mb);
    if (error) {
        llvm::errs() << "Failed to read: " << inputFile << "\n";
        return false;
    }

    builder = createDefaultExprBuilder();
    Parser *parser = Parser::Create(inputFile.c_str(), mb.get(), builder);
    parser->SetMaxErrors(BIPATH_MAX_PARSER_ERROR);

    std::vector< ref<Expr> > retConstraints;
    llvm::errs() << "Testing: " << inputFile << "\n";
    while (Decl *decl = parser->ParseTopLevelDecl()) {
        llvm::errs() << "\t====\n\tFound Decl.\n";
        if (decl->getKind() != Decl::QueryCommandDeclKind) {
            continue;
        }
        QueryCommand *qc;
        qc = static_cast<QueryCommand*>(decl);
        if (qc->Constraints.size() == 0) {
            continue;
        }
        else if (qc->Constraints.size() == 1) {
            retConstraints.push_back(qc->Constraints[0]);
            continue;
        }

        ref<Expr> arg1 = qc->Constraints[0];
        ref<Expr> arg2 = qc->Constraints[1];
        Expr::CreateArg carg2(qc->Constraints[0]);
        errs() << carg2.isExpr() << "==\n";
        ref<Expr> currConstr = mergeConstraints(qc->Constraints[0],
                qc->Constraints[1]);

        for (unsigned int i = 2; i < qc->Constraints.size() - 1; i++) {
            //ref<Expr> newConst = mergeConstraints(currConstr, qc->Constraints[i]);
            currConstr = mergeConstraints(currConstr, qc->Constraints[i]);
        }
        retConstraints.push_back(currConstr);
    }
    for (int i = 0; i < retConstraints.size(); i++) {
        ExprPPrinter::printSingleExpr(errs(), retConstraints[i]);
    }
    return true;
}

int main(int argc, char **argv) {

    /****************************************
     * Start CL Parsing
     * TODO: Use llvm ParseCommandLineOptions
     ***************************************/
    if (argc != 2) {
        llvm::errs() << "Expected: bipath <input dir>\n";
        return -1;
    }
    std::string inputDir(argv[1]);
    std::vector<std::string> pcsFiles;
    
    if (!getPCFiles(inputDir, &pcsFiles)) {
        llvm::errs() << "Failed to read pc files.\n";
        return -1;
    }


    std::vector<std::string>::iterator iter;
    for (unsigned int i = 0; i < pcsFiles.size(); i++) {
        rewriteConstraints(pcsFiles[i]);
    }

    return 1;
}
