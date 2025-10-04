#include "codeparser/ast.h"

namespace Iodicium {
    namespace Codeparser {

        ImportStmt::ImportStmt(Token path)
            : path(std::move(path)) {}

        void ImportStmt::accept(StmtVisitor& visitor) const {
            visitor.visit(*this);
        }


        

        FunctionDeclStmt::FunctionDeclStmt(Token name, std::vector<Parameter> params, std::unique_ptr<Expr> return_type, bool is_exported)
            : name(std::move(name)), params(std::move(params)), return_type_expr(std::move(return_type)), is_exported(is_exported) {}

        void FunctionDeclStmt::accept(StmtVisitor& visitor) const {
            visitor.visit(*this);
        }

        ReturnStmt::ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
            : keyword(std::move(keyword)), value(std::move(value)) {}

        void ReturnStmt::accept(StmtVisitor& visitor) const {
            visitor.visit(*this);
        }

        ExprStmt::ExprStmt(std::unique_ptr<Expr> expression) : expression(std::move(expression)) {}

        void ExprStmt::accept(StmtVisitor& visitor) const {
            visitor.visit(*this);
        }

        VarStmt::VarStmt(Token name, std::unique_ptr<Expr> type_expr, std::unique_ptr<Expr> initializer, bool is_mutable, bool is_exported)
            : name(std::move(name)), type_expr(std::move(type_expr)), initializer(std::move(initializer)), is_mutable(is_mutable), is_exported(is_exported) {}

        void VarStmt::accept(StmtVisitor& visitor) const {
            visitor.visit(*this);
        }

    }
}
