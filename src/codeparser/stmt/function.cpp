#include "codeparser/ast.h"

namespace Iodicium {
    namespace Codeparser {

        // FunctionStmt
        FunctionStmt::FunctionStmt(Token name, std::vector<Token> params, std::vector<std::unique_ptr<Stmt>> body, bool is_exported)
            : name(std::move(name)), params(std::move(params)), body(std::move(body)), is_exported(is_exported) {}

        void FunctionStmt::accept(StmtVisitor& visitor) const {
            visitor.visit(*this);
        }

    }
}
