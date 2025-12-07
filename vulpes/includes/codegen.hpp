#pragma once
#include "ast.hpp"
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

struct VariableInfo {
    std::string address;
    std::string type;
};

struct FunctionInfo {
    std::string key;      // lookup key (ns.name or name)
    std::string irName;   // LLVM-visible name
    std::string returnType;
    std::vector<Parameter> parameters;
    FunctionDefinition* definition;
};

class CodeGenerator {
public:
    CodeGenerator();
    std::string generate(const std::vector<std::unique_ptr<Statement>>& statements);

private:
    int tempCounter;
    int strCounter;
    int labelCounter;

    std::ostringstream globals;
    std::ostringstream body;
    std::unordered_map<std::string, FunctionInfo> functions;

    struct Scope {
        std::unordered_map<std::string, VariableInfo> variables;
    };
    std::vector<Scope> scopes;

    // helper utilities
    std::string nextTemp();
    std::string nextStringName();
    std::string nextLabel(const std::string& base);
    std::string mapType(const std::string& type) const;
    VariableInfo* resolveVariable(const std::string& name);
    void pushScope();
    void popScope();

    // generation
    void registerFunction(FunctionDefinition* func);
    void registerImportedFunctions(const std::vector<std::unique_ptr<Statement>>& module, const std::string& ns);
    void emitBuiltins(std::ostringstream& out);
    void emitFormatGlobals();
    std::string emitFunction(FunctionDefinition* func, const std::string& irName);
    bool emitStatement(Statement* stmt, const std::string& currentReturn);
    std::string emitExpression(Expression* expr, std::string& outType);
    std::string convert(const std::string& value, const std::string& from, const std::string& to);
};
