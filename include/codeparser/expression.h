#ifndef IODICIUM_CODEPARSER_EXPRESSION_H
#define IODICIUM_CODEPARSER_EXPRESSION_H

#include <memory>
#include <vector>
#include "codeparser/tokenizer.h" // Direct include

namespace Iodicium {
    namespace Codeparser {

        // Forward declare all expression types
        struct BinaryExpr;
        struct GroupingExpr;
        struct LiteralExpr;
        struct VariableExpr;
        struct CallExpr;
        struct AssignExpr;

        // Visitor interface for expressions
        struct ExprVisitor {
            virtual ~ExprVisitor() = default;
            virtual void visit(const BinaryExpr& expr) = 0;
            virtual void visit(const GroupingExpr& expr) = 0;
            virtual void visit(const LiteralExpr& expr) = 0; // Fixed typo
            virtual void visit(const VariableExpr& expr) = 0;
            virtual void visit(const CallExpr& expr) = 0;
            virtual void visit(const AssignExpr& expr) = 0;
        };

        // Base class for all expressions
        struct Expr {
            Token token; // Added token member to base class
            virtual ~Expr() = default;
            virtual void accept(ExprVisitor& visitor) const = 0;
        protected:
            explicit Expr(Token token) : token(std::move(token)) {}
        };

        struct BinaryExpr : Expr {
            std::unique_ptr<Expr> left;
            Token op; // Uses fully qualified name via include
            std::unique_ptr<Expr> right;

            BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right);
            void accept(ExprVisitor& visitor) const override;
        };

        struct GroupingExpr : Expr {
            std::unique_ptr<Expr> expression;

            GroupingExpr(Token paren, std::unique_ptr<Expr> expression); // Updated constructor
            void accept(ExprVisitor& visitor) const override;
        };

        struct LiteralExpr : Expr {
            Token value;

            explicit LiteralExpr(Token value);
            void accept(ExprVisitor& visitor) const override;
        };

        struct VariableExpr : Expr {
            Token name;

            explicit VariableExpr(Token name);
            void accept(ExprVisitor& visitor) const override;
        };

        struct CallExpr : Expr {
            std::unique_ptr<Expr> callee;
            Token paren;
            std::vector<std::unique_ptr<Expr>> arguments;

            CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments);
            void accept(ExprVisitor& visitor) const override;
        };

        struct AssignExpr : Expr {
            Token name;
            std::unique_ptr<Expr> value;

            AssignExpr(Token name, std::unique_ptr<Expr> value);
            void accept(ExprVisitor& visitor) const override;
        };

    }
}

#endif //IODICIUM_CODEPARSER_EXPRESSION_H
