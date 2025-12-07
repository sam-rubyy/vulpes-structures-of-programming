#include "parser.hpp"

#include <stdexcept>
#include <utility>

Parser::Parser(const std::vector<Token>& tokens, ErrorHandler& handler)
    : tokens(tokens), pos(0), errorHandler(handler) {}

const Token& Parser::current() const {
    return tokens[pos];
}

bool Parser::match(TokenType type) {
    if (current().type == type) {
        advance();
        return true;
    }
    return false;
}

void Parser::advance() {
    if (!isAtEnd()) {
        pos++;
    }
}

bool Parser::isAtEnd() const {
    return current().type == TokenType::EndOfFile;
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!match(type)) {
        errorHandler.error(current().line, current().column, message);
        throw std::runtime_error("parse error");
    }
}

void Parser::synchronize() {
    while (!isAtEnd()) {
        if (tokens[pos - 1].type == TokenType::Semicolon) return;
        switch (current().type) {
            case TokenType::Fx:
            case TokenType::Var:
            case TokenType::Const:
            case TokenType::If:
            case TokenType::For:
            case TokenType::While:
            case TokenType::Return:
                return;
            default:
                break;
        }
        advance();
    }
}

std::vector<std::unique_ptr<Statement>> Parser::parseProgram() {
    std::vector<std::unique_ptr<Statement>> program;
    while (!isAtEnd()) {
        try {
            auto decl = declaration();
            if (decl) program.push_back(std::move(decl));
        } catch (...) {
            synchronize();
        }
    }
    return program;
}

std::unique_ptr<Statement> Parser::declaration() {
    if (match(TokenType::Mod)) return moduleImport();
    if (match(TokenType::Fx)) return functionDefinition();
    if (match(TokenType::Var)) return varDeclaration(false);
    if (match(TokenType::Const)) return varDeclaration(true);
    return statement();
}

std::unique_ptr<Statement> Parser::moduleImport() {
    expect(TokenType::LeftParen, "expected '(' after mod");
    if (current().type != TokenType::String) {
        errorHandler.error(current().line, current().column, "expected string path in module import");
        throw std::runtime_error("parse error");
    }
    std::string path = current().lexeme;
    advance();
    expect(TokenType::RightParen, "expected ')' after module path");
    expect(TokenType::ColonColon, "expected '::' for module alias");
    if (current().type != TokenType::Identifier) {
        errorHandler.error(current().line, current().column, "expected module alias identifier");
        throw std::runtime_error("parse error");
    }
    std::string alias = current().lexeme;
    advance();
    expect(TokenType::Semicolon, "expected ';' after module import");
    auto stmt = std::make_unique<ModuleImport>();
    stmt->path = path;
    stmt->alias = alias;
    return stmt;
}

std::unique_ptr<Statement> Parser::functionDefinition() {
    if (current().type != TokenType::Identifier) {
        errorHandler.error(current().line, current().column, "expected function name");
        throw std::runtime_error("parse error");
    }
    std::string name = current().lexeme;
    advance();
    expect(TokenType::LeftParen, "expected '(' after function name");
    std::vector<Parameter> params;
    if (!match(TokenType::RightParen)) {
        do {
            if (current().type != TokenType::Identifier) {
                errorHandler.error(current().line, current().column, "expected parameter type");
                throw std::runtime_error("parse error");
            }
            Parameter param;
            param.type = current().lexeme;
            advance();
            if (match(TokenType::Colon)) {
                if (current().type != TokenType::Identifier) {
                    errorHandler.error(current().line, current().column, "expected parameter name");
                    throw std::runtime_error("parse error");
                }
                param.name = current().lexeme;
                advance();
            } else {
                param.name = "p" + std::to_string(params.size());
            }
            params.push_back(param);
        } while (match(TokenType::Comma));
        expect(TokenType::RightParen, "expected ')' after parameters");
    }

    std::string returnType = "void";
    if (match(TokenType::Arrow)) {
        if (current().type != TokenType::Identifier) {
            errorHandler.error(current().line, current().column, "expected return type");
            throw std::runtime_error("parse error");
        }
        returnType = current().lexeme;
        advance();
    }

    if (match(TokenType::Semicolon)) {
        // Prototype only; skip emitting a body
        return nullptr;
    }
    auto body = block();
    auto func = std::make_unique<FunctionDefinition>();
    func->name = name;
    func->returnType = returnType;
    func->parameters = std::move(params);
    func->body = std::move(body);
    return func;
}

std::unique_ptr<Statement> Parser::varDeclaration(bool isConst) {
    std::string type;
    if (match(TokenType::ColonColon)) {
        if (current().type != TokenType::Identifier) {
            errorHandler.error(current().line, current().column, "expected type after '::'");
            throw std::runtime_error("parse error");
        }
        type = current().lexeme;
        advance();
    }
    if (current().type != TokenType::Identifier) {
        errorHandler.error(current().line, current().column, "expected variable name");
        throw std::runtime_error("parse error");
    }
    std::string name = current().lexeme;
    advance();
    std::unique_ptr<Expression> init;
    if (match(TokenType::Assign)) {
        init = expression();
    }
    expect(TokenType::Semicolon, "expected ';' after variable declaration");
    return std::make_unique<VariableDeclaration>(name, type, isConst, std::move(init));
}

std::unique_ptr<BlockStatement> Parser::block() {
    expect(TokenType::LeftBrace, "expected '{'");
    auto blk = std::make_unique<BlockStatement>();
    while (!isAtEnd() && current().type != TokenType::RightBrace) {
        auto stmt = declaration();
        if (stmt) blk->statements.push_back(std::move(stmt));
    }
    expect(TokenType::RightBrace, "expected '}'");
    return blk;
}

std::unique_ptr<Statement> Parser::statement() {
    if (match(TokenType::If)) return ifStatement();
    if (match(TokenType::For)) return forStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::Print)) return printStatement();
    if (match(TokenType::Gather)) return gatherStatement();
    if (match(TokenType::LeftBrace)) {
        pos--; // rewind so block() can consume '{'
        return block();
    }

    auto expr = expression();
    expect(TokenType::Semicolon, "expected ';' after expression");
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

std::unique_ptr<Statement> Parser::ifStatement() {
    expect(TokenType::LeftParen, "expected '(' after if");
    auto cond = expression();
    expect(TokenType::RightParen, "expected ')' after condition");
    auto thenBranch = block();
    std::unique_ptr<BlockStatement> elseBranch;
    if (match(TokenType::Else)) {
        elseBranch = block();
    }
    auto stmt = std::make_unique<IfStatement>();
    stmt->condition = std::move(cond);
    stmt->thenBranch = std::move(thenBranch);
    stmt->elseBranch = std::move(elseBranch);
    return stmt;
}

std::unique_ptr<Statement> Parser::forStatement() {
    if (current().type != TokenType::Identifier) {
        errorHandler.error(current().line, current().column, "expected iterator name");
        throw std::runtime_error("parse error");
    }
    std::string iterator = current().lexeme;
    advance();
    expect(TokenType::In, "expected 'in' after iterator");
    auto start = expression();
    expect(TokenType::DotDot, "expected '..' in range");
    auto end = expression();
    auto bodyBlock = block();
    auto stmt = std::make_unique<ForStatement>();
    stmt->iterator = iterator;
    stmt->start = std::move(start);
    stmt->end = std::move(end);
    stmt->body = std::move(bodyBlock);
    return stmt;
}

std::unique_ptr<Statement> Parser::whileStatement() {
    expect(TokenType::LeftParen, "expected '(' after while");
    auto cond = expression();
    expect(TokenType::RightParen, "expected ')' after condition");
    auto bodyBlock = block();
    auto stmt = std::make_unique<WhileStatement>();
    stmt->condition = std::move(cond);
    stmt->body = std::move(bodyBlock);
    return stmt;
}

std::unique_ptr<Statement> Parser::returnStatement() {
    std::unique_ptr<Expression> expr;
    if (current().type != TokenType::Semicolon) {
        expr = expression();
    }
    expect(TokenType::Semicolon, "expected ';' after return");
    return std::make_unique<ReturnStatement>(std::move(expr));
}

std::unique_ptr<Statement> Parser::printStatement() {
    expect(TokenType::LeftParen, "expected '(' after print");
    std::vector<std::unique_ptr<Expression>> args;
    if (!match(TokenType::RightParen)) {
        do {
            args.push_back(expression());
        } while (match(TokenType::Comma));
        expect(TokenType::RightParen, "expected ')' after print arguments");
    }
    expect(TokenType::Semicolon, "expected ';' after print");

    bool formatted = false;
    std::string fmt;
    std::vector<std::unique_ptr<Expression>> realArgs;
    if (!args.empty()) {
        if (auto* str = dynamic_cast<StringExpression*>(args[0].get())) {
            formatted = true;
            fmt = str->value;
            for (size_t i = 1; i < args.size(); ++i) {
                realArgs.push_back(std::move(args[i]));
            }
        } else {
            fmt = "{}";
            realArgs.push_back(std::move(args[0]));
        }
    }
    return std::make_unique<PrintStatement>(fmt, std::move(realArgs), formatted);
}

std::unique_ptr<Statement> Parser::gatherStatement() {
    expect(TokenType::LeftParen, "expected '(' after gather");
    std::vector<std::string> names;
    if (!match(TokenType::RightParen)) {
        do {
            if (current().type != TokenType::Identifier) {
                errorHandler.error(current().line, current().column, "expected identifier in gather");
                throw std::runtime_error("parse error");
            }
            names.push_back(current().lexeme);
            advance();
        } while (match(TokenType::Comma));
        expect(TokenType::RightParen, "expected ')' after gather list");
    }
    expect(TokenType::Semicolon, "expected ';' after gather");
    auto stmt = std::make_unique<GatherStatement>();
    stmt->names = std::move(names);
    return stmt;
}

std::unique_ptr<Expression> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expression> Parser::assignment() {
    auto expr = comparison();
    if (match(TokenType::Assign)) {
        if (auto* var = dynamic_cast<VariableExpression*>(expr.get())) {
            std::string name = var->name;
            auto value = assignment();
            return std::make_unique<AssignmentExpression>(name, std::move(value));
        }
        errorHandler.error(current().line, current().column, "invalid assignment target");
        throw std::runtime_error("parse error");
    }
    return expr;
}

std::unique_ptr<Expression> Parser::comparison() {
    auto expr = term();
    while (true) {
        TokenType t = current().type;
        if (t == TokenType::Equals || t == TokenType::NotEquals ||
            t == TokenType::Less || t == TokenType::LessEq ||
            t == TokenType::Greater || t == TokenType::GreaterEq) {
            std::string op = current().lexeme;
            advance();
            auto right = term();
            expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
        } else break;
    }
    return expr;
}

std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();
    while (current().type == TokenType::Plus || current().type == TokenType::Minus) {
        std::string op = current().lexeme;
        advance();
        auto right = factor();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::factor() {
    auto expr = unary();
    while (current().type == TokenType::Star || current().type == TokenType::Slash) {
        std::string op = current().lexeme;
        advance();
        auto right = unary();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::unary() {
    if (match(TokenType::Minus)) {
        auto operand = unary();
        return std::make_unique<UnaryExpression>("-", std::move(operand));
    }
    return call();
}

std::unique_ptr<Expression> Parser::call() {
    auto expr = primary();
    return expr;
}

std::unique_ptr<Expression> Parser::primary() {
    if (current().type == TokenType::Number) {
        int value = std::stoi(current().lexeme);
        advance();
        return std::make_unique<NumberExpression>(value);
    }
    if (current().type == TokenType::Float) {
        double value = std::stod(current().lexeme);
        advance();
        return std::make_unique<FloatExpression>(value);
    }
    if (current().type == TokenType::String) {
        std::string value = current().lexeme;
        advance();
        return std::make_unique<StringExpression>(value);
    }
    if (current().type == TokenType::True || current().type == TokenType::False) {
        bool v = current().type == TokenType::True;
        advance();
        return std::make_unique<BoolExpression>(v);
    }
    if (current().type == TokenType::Identifier) {
        std::string name = current().lexeme;
        advance();
        std::string ns;
        if (match(TokenType::Dot)) {
            if (current().type != TokenType::Identifier) {
                errorHandler.error(current().line, current().column, "expected member after '.'");
                throw std::runtime_error("parse error");
            }
            ns = name;
            name = current().lexeme;
            advance();
        }
        if (match(TokenType::LeftParen)) {
            std::vector<std::unique_ptr<Expression>> args;
            if (!match(TokenType::RightParen)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::Comma));
                expect(TokenType::RightParen, "expected ')' after arguments");
            }
            return std::make_unique<CallExpression>(name, std::move(args), ns);
        }
        if (!ns.empty()) {
            errorHandler.error(current().line, current().column, "namespaced value must be a call");
            throw std::runtime_error("parse error");
        }
        return std::make_unique<VariableExpression>(name);
    }
    if (match(TokenType::LeftParen)) {
        auto expr = expression();
        expect(TokenType::RightParen, "expected ')'");
        return expr;
    }

    errorHandler.error(current().line, current().column, "unexpected token");
    throw std::runtime_error("parse error");
}
