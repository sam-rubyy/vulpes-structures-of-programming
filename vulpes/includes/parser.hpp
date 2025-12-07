#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include "error_handler.hpp"
#include <memory>
#include <vector>

class Parser {
public:
    Parser(const std::vector<Token>& tokens, ErrorHandler& handler);
    std::vector<std::unique_ptr<Statement>> parseProgram();

private:
    const std::vector<Token>& tokens;
    size_t pos;
    ErrorHandler& errorHandler;

    const Token& current() const;
    bool match(TokenType type);
    void advance();
    bool isAtEnd() const;

    std::unique_ptr<Statement> declaration();
    std::unique_ptr<Statement> statement();
    std::unique_ptr<BlockStatement> block();
    std::unique_ptr<Statement> ifStatement();
    std::unique_ptr<Statement> forStatement();
    std::unique_ptr<Statement> whileStatement();
    std::unique_ptr<Statement> returnStatement();
    std::unique_ptr<Statement> printStatement();
    std::unique_ptr<Statement> gatherStatement();
    std::unique_ptr<Statement> varDeclaration(bool isConst);
    std::unique_ptr<Statement> functionDefinition();
    std::unique_ptr<Statement> moduleImport();

    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> assignment();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> call();
    std::unique_ptr<Expression> primary();

    void synchronize();
    void expect(TokenType type, const std::string& message);
};
