#ifndef IODICIUM_CODEPARSER_AST_H
#define IODICIUM_CODEPARSER_AST_H

#include <memory>
#include <vector>
#include "codeparser/tokenizer.h"
#include "codeparser/expression.h"

namespace Iodicium {
    namespace Codeparser {

        struct FunctionStmt;
        struct FunctionDeclStmt;
        struct ReturnStmt;
        struct ExprStmt;
        struct VarStmt;
        struct ImportStmt;

        struct StmtVisitor {
            virtual ~StmtVisitor() = default;
            virtual void visit(const FunctionStmt& stmt) = 0;
            virtual void visit(const FunctionDeclStmt& stmt) = 0;
            virtual void visit(const ReturnStmt& stmt) = 0;
            virtual void visit(const ExprStmt& stmt) = 0;
            virtual void visit(const VarStmt& stmt) = 0;
            virtual void visit(const ImportStmt& stmt) = 0;
        };

        struct Stmt {
            virtual ~Stmt() = default;
            virtual void accept(StmtVisitor& visitor) const = 0;
        };

        // New struct to hold a parameter's name and type
        struct Parameter {
            Token name;
            std::unique_ptr<Expr> type_expr;
        };

        struct ImportStmt : Stmt {
            Token path;
            explicit ImportStmt(Token path);
            void accept(StmtVisitor& visitor) const override;
        };

        struct FunctionStmt : Stmt {
            Token name;
            std::vector<Parameter> params; // Changed from std::vector<Token>
            std::unique_ptr<Expr> return_type_expr;
            std::vector<std::unique_ptr<Stmt>> body;
            bool is_exported;

            FunctionStmt(Token name, std::vector<Parameter> params, std::unique_ptr<Expr> return_type, std::vector<std::unique_ptr<Stmt>> body, bool is_exported);
            void accept(StmtVisitor& visitor) const override;
        };

        struct FunctionDeclStmt : Stmt {
            Token name;
            std::vector<Parameter> params; // Changed from std::vector<Token>
            std::unique_ptr<Expr> return_type_expr;
            bool is_exported;

            FunctionDeclStmt(Token name, std::vector<Parameter> params, std::unique_ptr<Expr> return_type, bool is_exported);
            void accept(StmtVisitor& visitor) const override;
        };

        struct ReturnStmt : Stmt {
            Token keyword;
            std::unique_ptr<Expr> value;
            ReturnStmt(Token keyword, std::unique_ptr<Expr> value);
            void accept(StmtVisitor& visitor) const override;
        };

        struct ExprStmt : Stmt {
            std::unique_ptr<Expr> expression;
            explicit ExprStmt(std::unique_ptr<Expr> expression);
            void accept(StmtVisitor& visitor) const override;
        };

        struct VarStmt : Stmt {
            Token name;
            std::unique_ptr<Expr> type_expr;
            std::unique_ptr<Expr> initializer;
            bool is_mutable;
            bool is_exported;

            VarStmt(Token name, std::unique_ptr<Expr> type_expr, std::unique_ptr<Expr> initializer, bool is_mutable, bool is_exported);
            void accept(StmtVisitor& visitor) const override;
        };

    }
}

#endif //IODICIUM_CODEPARSER_AST_H
