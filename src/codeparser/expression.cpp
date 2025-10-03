#include "codeparser/expression.h"

namespace Iodicium {
    namespace Codeparser {

        // BinaryExpr
        BinaryExpr::BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
            : Expr(op), left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

        void BinaryExpr::accept(ExprVisitor& visitor) const {
            visitor.visit(*this);
        }

        // GroupingExpr
        GroupingExpr::GroupingExpr(Token paren, std::unique_ptr<Expr> expression)
            : Expr(paren), expression(std::move(expression)) {}

        void GroupingExpr::accept(ExprVisitor& visitor) const {
            visitor.visit(*this);
        }

        // LiteralExpr
        LiteralExpr::LiteralExpr(Token value) : Expr(value), value(std::move(value)) {}

        void LiteralExpr::accept(ExprVisitor& visitor) const {
            visitor.visit(*this);
        }

        // VariableExpr
        VariableExpr::VariableExpr(Token name) : Expr(name), name(std::move(name)) {}

        void VariableExpr::accept(ExprVisitor& visitor) const {
            visitor.visit(*this);
        }

        // CallExpr
        CallExpr::CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
            : Expr(paren), callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}

        void CallExpr::accept(ExprVisitor& visitor) const {
            visitor.visit(*this);
        }

        // AssignExpr
        AssignExpr::AssignExpr(Token name, std::unique_ptr<Expr> value)
            : Expr(name), name(std::move(name)), value(std::move(value)) {}

        void AssignExpr::accept(ExprVisitor& visitor) const {
            visitor.visit(*this);
        }

    }
}
