#include "codeparser/ast.h"

namespace Iodicium {
    namespace Codeparser {

        FunctionStmt::FunctionStmt(Token name, std::vector<Parameter> params, std::unique_ptr<Expr> return_type, std::vector<std::unique_ptr<Stmt>> body, bool is_exported)
            : name(std::move(name)), params(std::move(params)), return_type_expr(std::move(return_type)), body(std::move(body)), is_exported(is_exported) {}

        void FunctionStmt::accept(StmtVisitor& visitor) const {
            visitor.visit(*this);
        }

    }
}
