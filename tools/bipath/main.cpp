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
//static ref<Expr> mergeConstraints(Expr::CreateArg arg1, Expr::CreateArg arg2)
{
    std::vector<Expr::CreateArg> args;
    Expr::CreateArg arg1(constr1);
    Expr::CreateArg arg2(constr2);
    args.push_back(arg1);
    args.push_back(arg2);
    return Expr::createFromKind(Expr::And, args);
}

static bool rewriteConstraints(const std::string inputFile,
        std::vector<Decl*> *retDecls)
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

    while (Decl *decl = parser->ParseTopLevelDecl()) {
        if (decl->getKind() != Decl::QueryCommandDeclKind) {
            retDecls->push_back(decl);
            continue;
        }

        std::vector< ref<Expr> > newQuery;
        QueryCommand *qc;
        qc = static_cast<QueryCommand*>(decl);
        if (qc->Constraints.size() == 0) {
            continue;
        }
        else if (qc->Constraints.size() == 1) {
            newQuery.push_back(qc->Constraints[0]);
            continue;
        }

        ref<Expr> currConstr = mergeConstraints(qc->Constraints[0],
                qc->Constraints[1]);
        for (unsigned int i = 2; i < qc->Constraints.size() - 1; i++) {
            currConstr = mergeConstraints(currConstr, qc->Constraints[i]);
        }
        newQuery.push_back(currConstr);

        std::vector< ref<Expr> > newConstraints;
        std::vector< ref<Expr> > newValues;
        std::vector< const Array * > newObjects;
        QueryCommand *newQC = new QueryCommand(newConstraints, newQuery[0],
                newValues, newObjects);
        retDecls->push_back(newQC);

    }

    return true;
}

static bool writeNewPCFile(std::string newFile, std::vector<Decl*> *decls)
{
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
        std::vector<Decl*> newDecls;
        rewriteConstraints(pcsFiles[i], &newDecls);
        writeNewPCFile(pcsFiles[i] + ".new", &newDecls);
    }

    return 1;
}
