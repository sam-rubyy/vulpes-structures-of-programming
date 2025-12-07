#pragma once
#include <memory>
#include <string>
#include <vector>

struct Parameter {
    std::string type;
    std::string name;
};

// Base nodes
struct ASTNode {
    virtual ~ASTNode() = default;
};

struct Expression : ASTNode {
    virtual ~Expression() = default;
};

struct Statement : ASTNode {
    virtual ~Statement() = default;
};

// Expressions
struct NumberExpression : Expression {
    int value;
    explicit NumberExpression(int v) : value(v) {}
};

struct FloatExpression : Expression {
    double value;
    explicit FloatExpression(double v) : value(v) {}
};

struct StringExpression : Expression {
    std::string value;
    explicit StringExpression(std::string v) : value(std::move(v)) {}
};

struct BoolExpression : Expression {
    bool value;
    explicit BoolExpression(bool v) : value(v) {}
};

struct VariableExpression : Expression {
    std::string name;
    explicit VariableExpression(std::string n) : name(std::move(n)) {}
};

struct UnaryExpression : Expression {
    std::string op;
    std::unique_ptr<Expression> operand;
    UnaryExpression(std::string o, std::unique_ptr<Expression> expr)
        : op(std::move(o)), operand(std::move(expr)) {}
};

struct BinaryExpression : Expression {
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    BinaryExpression(std::unique_ptr<Expression> l, std::string o, std::unique_ptr<Expression> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};

struct AssignmentExpression : Expression {
    std::string name;
    std::unique_ptr<Expression> value;
    AssignmentExpression(std::string n, std::unique_ptr<Expression> v)
        : name(std::move(n)), value(std::move(v)) {}
};

struct CallExpression : Expression {
    std::string name;
    std::string ns;
    std::vector<std::unique_ptr<Expression>> arguments;
    CallExpression(std::string n, std::vector<std::unique_ptr<Expression>> args, std::string nsName = "")
        : name(std::move(n)), ns(std::move(nsName)), arguments(std::move(args)) {}
};

// Statements
struct BlockStatement : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
};

struct VariableDeclaration : Statement {
    std::string name;
    std::string type;
    bool isConst;
    std::unique_ptr<Expression> initializer;
    VariableDeclaration(std::string n, std::string t, bool c, std::unique_ptr<Expression> init)
        : name(std::move(n)), type(std::move(t)), isConst(c), initializer(std::move(init)) {}
};

struct AssignmentStatement : Statement {
    std::string name;
    std::unique_ptr<Expression> value;
    AssignmentStatement(std::string n, std::unique_ptr<Expression> v)
        : name(std::move(n)), value(std::move(v)) {}
};

struct ExpressionStatement : Statement {
    std::unique_ptr<Expression> expression;
    explicit ExpressionStatement(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
};

struct ReturnStatement : Statement {
    std::unique_ptr<Expression> expression;
    explicit ReturnStatement(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
};

struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> thenBranch;
    std::unique_ptr<BlockStatement> elseBranch;
};

struct ForStatement : Statement {
    std::string iterator;
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> end;
    std::unique_ptr<BlockStatement> body;
};

struct WhileStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> body;
};

struct PrintStatement : Statement {
    std::string format;
    std::vector<std::unique_ptr<Expression>> arguments;
    bool formatted;
    PrintStatement(std::string fmt, std::vector<std::unique_ptr<Expression>> args, bool isFormatted)
        : format(std::move(fmt)), arguments(std::move(args)), formatted(isFormatted) {}
};

struct GatherStatement : Statement {
    std::vector<std::string> names;
};

struct FunctionDefinition : Statement {
    std::string name;
    std::string ns;
    std::string returnType;
    std::vector<Parameter> parameters;
    std::unique_ptr<BlockStatement> body;
};

struct ModuleImport : Statement {
    std::string path;
    std::string alias;
};
