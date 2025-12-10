#include "codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "error_handler.hpp"

#include <fstream>
#include <sstream>

namespace {
int alignmentFor(const std::string& llvmType) {
    if (llvmType == "double") return 8;
    if (llvmType == "i8*" || llvmType == "i64") return 8;
    return 4;
}

std::string escapeString(const std::string& value) {
    std::string out;
    for (char c : value) {
        switch (c) {
            case '\n': out += "\\0A"; break;
            case '\t': out += "\\09"; break;
            case '\r': out += "\\0D"; break;
            case '\\': out += "\\5C"; break;
            case '"': out += "\\22"; break;
            default: out.push_back(c); break;
        }
    }
    out += "\\00";
    return out;
}
} // namespace

CodeGenerator::CodeGenerator()
    : tempCounter(0), strCounter(0), labelCounter(0) {}

std::string CodeGenerator::nextTemp() {
    return "%t" + std::to_string(++tempCounter);
}

std::string CodeGenerator::nextStringName() {
    return ".str" + std::to_string(++strCounter);
}

std::string CodeGenerator::nextLabel(const std::string& base) {
    return base + "_" + std::to_string(++labelCounter);
}

void CodeGenerator::pushScope() {
    scopes.push_back({});
}

void CodeGenerator::popScope() {
    if (!scopes.empty()) scopes.pop_back();
}

VariableInfo* CodeGenerator::resolveVariable(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->variables.find(name);
        if (found != it->variables.end()) return &found->second;
    }
    return nullptr;
}

std::string CodeGenerator::mapType(const std::string& type) const {
    if (type == "int" || type.empty()) return "i32";
    if (type == "float") return "double";
    if (type == "bool") return "i1";
    if (type == "string") return "i8*";
    if (type == "void") return "void";
    return "i32";
}

void CodeGenerator::emitBuiltins(std::ostringstream& out) {
    out << "; ModuleID = 'vulpes_module'\n";
    out << "target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n";
    out << "target triple = \"x86_64-pc-linux-gnu\"\n\n";
    out << "declare i32 @printf(i8*, ...)\n";
    out << "declare i32 @scanf(i8*, ...)\n";
    out << "declare double @sqrt(double)\n";
    out << "declare i64 @time(i8*)\n\n";
    out << "@.str_int = private unnamed_addr constant [4 x i8] c\"%d\\0A\\00\", align 1\n";
    out << "@.str_float = private unnamed_addr constant [4 x i8] c\"%g\\0A\\00\", align 1\n";
    out << "@.str_string = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\", align 1\n";
    out << "@.str_input_int = private unnamed_addr constant [3 x i8] c\"%d\\00\", align 1\n";
    out << "@.str_input_float = private unnamed_addr constant [4 x i8] c\"%lf\\00\", align 1\n";
    out << "@rand_seed = global i32 1, align 4\n";
    out << "@rand_seeded = global i1 false, align 1\n\n";
}

void CodeGenerator::emitFormatGlobals() {
    // Base globals already emitted in emitBuiltins; dynamic ones are collected in globals.
}

void CodeGenerator::registerFunction(FunctionDefinition* func) {
    std::string key = func->ns.empty() ? func->name : func->ns + "." + func->name;
    std::string irName = func->ns.empty() ? func->name : func->ns + "_" + func->name;
    FunctionInfo info;
    info.key = key;
    info.irName = irName;
    info.returnType = mapType(func->returnType);
    info.parameters = func->parameters;
    info.definition = func;
    functions[key] = info;
}

void CodeGenerator::registerImportedFunctions(const std::vector<std::unique_ptr<Statement>>& module, const std::string& ns) {
    for (const auto& stmt : module) {
        if (auto* func = dynamic_cast<FunctionDefinition*>(stmt.get())) {
            func->ns = ns;
            registerFunction(func);
        }
    }
}

std::string CodeGenerator::generate(const std::vector<std::unique_ptr<Statement>>& statements) {
    tempCounter = 0;
    strCounter = 0;
    labelCounter = 0;
    functions.clear();
    scopes.clear();
    globals.str("");
    globals.clear();

    // Load modules
    struct ModulePayload {
        std::string alias;
        std::vector<std::unique_ptr<Statement>> nodes;
    };
    std::vector<ModulePayload> modules;

    for (const auto& stmt : statements) {
        if (auto* mod = dynamic_cast<ModuleImport*>(stmt.get())) {
            std::ifstream file(mod->path);
            if (!file.is_open()) {
                continue;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            Lexer lx(content);
            ErrorHandler handler(content, mod->path);
            Parser parser(lx.tokens, handler);
            auto parsed = parser.parseProgram();
            if (handler.hasErrors()) {
                handler.printErrors();
            }
            modules.push_back({mod->alias, std::move(parsed)});
        }
    }

    // Register all functions (modules first so they can be referenced)
    for (auto& module : modules) {
        registerImportedFunctions(module.nodes, module.alias);
    }
    for (const auto& stmt : statements) {
        if (auto* func = dynamic_cast<FunctionDefinition*>(stmt.get())) {
            registerFunction(func);
        }
    }

    std::ostringstream header;
    emitBuiltins(header);
    std::vector<std::string> functionBlocks;

    // Generate functions
    for (auto& module : modules) {
        for (auto& stmt : module.nodes) {
            if (auto* func = dynamic_cast<FunctionDefinition*>(stmt.get())) {
                functionBlocks.push_back(emitFunction(func, functions[func->ns + "." + func->name].irName));
            }
        }
    }
    for (const auto& stmt : statements) {
        if (auto* func = dynamic_cast<FunctionDefinition*>(stmt.get())) {
            std::string key = func->ns.empty() ? func->name : func->ns + "." + func->name;
            functionBlocks.push_back(emitFunction(func, functions[key].irName));
        }
    }

    // Append dynamic globals after functions to keep IR compact
    std::string extraGlobals = globals.str();
    std::ostringstream ir;
    ir << header.str();
    if (!extraGlobals.empty()) ir << extraGlobals;
    for (const auto& block : functionBlocks) {
        ir << block << "\n";
    }
    if (functions.find("main") == functions.end()) {
        ir << "define i32 @main() {\n  ret i32 0\n}\n";
    }

    return ir.str();
}

std::string CodeGenerator::emitFunction(FunctionDefinition* func, const std::string& irName) {
    pushScope();
    body.str("");
    body.clear();

    std::ostringstream out;
    std::string retType = mapType(func->returnType);

    out << "define " << retType << " @" << irName << "(";
    for (size_t i = 0; i < func->parameters.size(); ++i) {
        if (i > 0) out << ", ";
        std::string paramType = mapType(func->parameters[i].type);
        out << paramType << " %" << func->parameters[i].name;
    }
    out << ") {\nentry:\n";

    // Allocate parameters locally so assignments work
    for (const auto& param : func->parameters) {
        std::string llvmType = mapType(param.type);
        std::string slot = nextTemp();
        int align = alignmentFor(llvmType);
        body << "  " << slot << " = alloca " << llvmType << ", align " << align << "\n";
        body << "  store " << llvmType << " %" << param.name << ", " << llvmType << "* " << slot << ", align " << align << "\n";
        scopes.back().variables[param.name] = {slot, llvmType};
    }

    bool returned = false;
    if (func->body) {
        for (const auto& stmt : func->body->statements) {
            returned = emitStatement(stmt.get(), retType);
            if (returned) break;
        }
    }

    // Default return if no explicit return emitted
    if (!returned) {
        if (retType == "void") {
            body << "  ret void\n";
        } else if (retType == "i32") {
            body << "  ret i32 0\n";
        } else if (retType == "double") {
            body << "  ret double 0.0\n";
        } else if (retType == "i1") {
            body << "  ret i1 false\n";
        } else if (retType == "i8*") {
            body << "  ret i8* null\n";
        }
    }

    out << body.str();
    out << "}\n";
    popScope();
    return out.str();
}

bool CodeGenerator::emitStatement(Statement* stmt, const std::string& currentReturn) {
    if (auto* block = dynamic_cast<BlockStatement*>(stmt)) {
        pushScope();
        for (const auto& s : block->statements) {
            if (emitStatement(s.get(), currentReturn)) {
                popScope();
                return true;
            }
        }
        popScope();
        return false;
    }

    if (auto* decl = dynamic_cast<VariableDeclaration*>(stmt)) {
        std::string initType = decl->type.empty() ? "" : mapType(decl->type);
        std::string value = "0";
        if (decl->initializer) {
            std::string exprType;
            value = emitExpression(decl->initializer.get(), exprType);
            if (initType.empty()) initType = exprType;
            else if (exprType != initType) value = convert(value, exprType, initType);
        } else {
            if (initType.empty()) initType = "i32";
            if (initType == "double") value = "0.0";
            else if (initType == "i1") value = "false";
            else value = "0";
        }
        std::string slot = nextTemp();
        int align = alignmentFor(initType);
        body << "  " << slot << " = alloca " << initType << ", align " << align << "\n";
        body << "  store " << initType << " " << value << ", " << initType << "* " << slot << ", align " << align << "\n";
        scopes.back().variables[decl->name] = {slot, initType};
        return false;
    }

    if (auto* assign = dynamic_cast<AssignmentStatement*>(stmt)) {
        auto* target = resolveVariable(assign->name);
        if (!target) return false;
        std::string rhsType;
        std::string rhs = emitExpression(assign->value.get(), rhsType);
        if (rhsType != target->type) rhs = convert(rhs, rhsType, target->type);
        int align = alignmentFor(target->type);
        body << "  store " << target->type << " " << rhs << ", " << target->type << "* " << target->address << ", align " << align << "\n";
        return false;
    }

    if (auto* exprStmt = dynamic_cast<ExpressionStatement*>(stmt)) {
        std::string type;
        emitExpression(exprStmt->expression.get(), type);
        return false;
    }

    if (auto* ret = dynamic_cast<ReturnStatement*>(stmt)) {
        if (ret->expression) {
            std::string type;
            std::string value = emitExpression(ret->expression.get(), type);
            if (type != currentReturn && currentReturn != "void") {
                value = convert(value, type, currentReturn);
                type = currentReturn;
            }
            body << "  ret " << type << " " << value << "\n";
        } else {
            body << "  ret void\n";
        }
        return true;
    }

    if (auto* print = dynamic_cast<PrintStatement*>(stmt)) {
        std::vector<std::pair<std::string, std::string>> args;
        for (const auto& arg : print->arguments) {
            std::string type;
            std::string val = emitExpression(arg.get(), type);
            args.push_back({val, type});
        }
        // Determine final format string
        std::string built = print->format;
        if (built.empty() && !args.empty()) {
            built = "{}";
        }
        std::string finalFmt;
        size_t argIndex = 0;
        for (size_t i = 0; i < built.size(); ++i) {
            if (built[i] == '{' && i + 1 < built.size() && built[i + 1] == '}' && argIndex < args.size()) {
                const auto& t = args[argIndex].second;
                if (t == "i32") finalFmt += "%d";
                else if (t == "double") finalFmt += "%g";
                else if (t == "i8*") finalFmt += "%s";
                else finalFmt += "%d";
                argIndex++;
                i++;
            } else {
                finalFmt.push_back(built[i]);
            }
        }
        while (argIndex < args.size()) {
            const auto& t = args[argIndex].second;
            if (!finalFmt.empty() && finalFmt.back() != ' ') finalFmt += " ";
            if (t == "i32") finalFmt += "%d";
            else if (t == "double") finalFmt += "%g";
            else if (t == "i8*") finalFmt += "%s";
            else finalFmt += "%d";
            argIndex++;
        }
        if (finalFmt.empty() || finalFmt.back() != '\n') {
            finalFmt.push_back('\n');
        }

        std::string escaped = escapeString(finalFmt); // escapeString appends null
        size_t length = finalFmt.size(); // includes newline
        std::string globalName = "@" + nextStringName();
        globals << globalName << " = private unnamed_addr constant [" << length + 1 << " x i8] c\"" << escaped << "\", align 1\n";

        std::string fmtPtr = nextTemp();
        body << "  " << fmtPtr << " = getelementptr inbounds [" << length + 1 << " x i8], [" << length + 1 << " x i8]* " << globalName << ", i32 0, i32 0\n";
        std::vector<std::pair<std::string, std::string>> convertedArgs;
        convertedArgs.reserve(args.size());
        for (auto& arg : args) {
            std::string type = arg.second;
            std::string val = arg.first;
            if (type == "i1") {
                val = convert(val, "i1", "i32");
                type = "i32";
            }
            convertedArgs.push_back({val, type});
        }

        body << "  " << nextTemp() << " = call i32 (i8*, ...) @printf(i8* " << fmtPtr;
        for (auto& arg : convertedArgs) {
            body << ", " << arg.second << " " << arg.first;
        }
        body << ")\n";
        return false;
    }

    if (auto* gather = dynamic_cast<GatherStatement*>(stmt)) {
        for (const auto& name : gather->names) {
            VariableInfo* var = resolveVariable(name);
            if (!var) {
                std::string slot = nextTemp();
                body << "  " << slot << " = alloca i32, align 4\n";
                body << "  store i32 0, i32* " << slot << ", align 4\n";
                scopes.back().variables[name] = {slot, "i32"};
                var = &scopes.back().variables[name];
            }
            std::string call = nextTemp();
            body << "  " << call << " = call i32 (i8*, ...) @scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str_input_int, i32 0, i32 0), i32* " << var->address << ")\n";
        }
        return false;
    }

    if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        std::string condType;
        std::string condVal = emitExpression(ifStmt->condition.get(), condType);
        if (condType != "i1") condVal = convert(condVal, condType, "i1");
        std::string thenLabel = nextLabel("if_then");
        std::string elseLabel = nextLabel("if_else");
        std::string endLabel = nextLabel("if_end");
        body << "  br i1 " << condVal << ", label %" << thenLabel << ", label %" << (ifStmt->elseBranch ? elseLabel : endLabel) << "\n";
        body << thenLabel << ":\n";
        emitStatement(ifStmt->thenBranch.get(), currentReturn);
        body << "  br label %" << endLabel << "\n";
        if (ifStmt->elseBranch) {
            body << elseLabel << ":\n";
            emitStatement(ifStmt->elseBranch.get(), currentReturn);
            body << "  br label %" << endLabel << "\n";
        }
        body << endLabel << ":\n";
        return false;
    }

    if (auto* whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        std::string condLabel = nextLabel("while_cond");
        std::string bodyLabel = nextLabel("while_body");
        std::string endLabel = nextLabel("while_end");
        body << "  br label %" << condLabel << "\n";
        body << condLabel << ":\n";
        std::string condType;
        std::string condVal = emitExpression(whileStmt->condition.get(), condType);
        if (condType != "i1") condVal = convert(condVal, condType, "i1");
        body << "  br i1 " << condVal << ", label %" << bodyLabel << ", label %" << endLabel << "\n";
        body << bodyLabel << ":\n";
        emitStatement(whileStmt->body.get(), currentReturn);
        body << "  br label %" << condLabel << "\n";
        body << endLabel << ":\n";
        return false;
    }

    if (auto* forStmt = dynamic_cast<ForStatement*>(stmt)) {
        std::string startType, endType;
        std::string startVal = emitExpression(forStmt->start.get(), startType);
        std::string endVal = emitExpression(forStmt->end.get(), endType);
        startVal = convert(startVal, startType, "i32");
        endVal = convert(endVal, endType, "i32");

        std::string iterSlot = nextTemp();
        body << "  " << iterSlot << " = alloca i32, align 4\n";
        body << "  store i32 " << startVal << ", i32* " << iterSlot << ", align 4\n";
        scopes.back().variables[forStmt->iterator] = {iterSlot, "i32"};

        std::string condLabel = nextLabel("for_cond");
        std::string loopLabel = nextLabel("for_body");
        std::string endLabel = nextLabel("for_end");

        body << "  br label %" << condLabel << "\n";
        body << condLabel << ":\n";
        std::string cur = nextTemp();
        body << "  " << cur << " = load i32, i32* " << iterSlot << ", align 4\n";
        std::string cmp = nextTemp();
        body << "  " << cmp << " = icmp slt i32 " << cur << ", " << endVal << "\n";
        body << "  br i1 " << cmp << ", label %" << loopLabel << ", label %" << endLabel << "\n";
        body << loopLabel << ":\n";
        emitStatement(forStmt->body.get(), currentReturn);
        std::string nextVal = nextTemp();
        body << "  " << nextVal << " = add i32 " << cur << ", 1\n";
        body << "  store i32 " << nextVal << ", i32* " << iterSlot << ", align 4\n";
        body << "  br label %" << condLabel << "\n";
        body << endLabel << ":\n";
        return false;
    }

    return false;
}

std::string CodeGenerator::emitExpression(Expression* expr, std::string& outType) {
    if (auto* num = dynamic_cast<NumberExpression*>(expr)) {
        outType = "i32";
        return std::to_string(num->value);
    }
    if (auto* fl = dynamic_cast<FloatExpression*>(expr)) {
        outType = "double";
        std::ostringstream oss;
        oss << fl->value;
        return oss.str();
    }
    if (auto* str = dynamic_cast<StringExpression*>(expr)) {
        std::string globalName = "@" + nextStringName();
        std::string escaped = escapeString(str->value);
        size_t length = str->value.size() + 1;
        globals << globalName << " = private unnamed_addr constant [" << length << " x i8] c\"" << escaped << "\", align 1\n";
        std::string ptr = nextTemp();
        body << "  " << ptr << " = getelementptr inbounds [" << length << " x i8], [" << length << " x i8]* " << globalName << ", i32 0, i32 0\n";
        outType = "i8*";
        return ptr;
    }
    if (auto* bl = dynamic_cast<BoolExpression*>(expr)) {
        outType = "i1";
        return bl->value ? "true" : "false";
    }
    if (auto* var = dynamic_cast<VariableExpression*>(expr)) {
        VariableInfo* info = resolveVariable(var->name);
        if (!info) {
            outType = "i32";
            return "0";
        }
        std::string tmp = nextTemp();
        body << "  " << tmp << " = load " << info->type << ", " << info->type << "* " << info->address << ", align " << alignmentFor(info->type) << "\n";
        outType = info->type;
        return tmp;
    }
    if (auto* unary = dynamic_cast<UnaryExpression*>(expr)) {
        std::string type;
        std::string val = emitExpression(unary->operand.get(), type);
        if (unary->op == "-") {
            std::string tmp = nextTemp();
            if (type == "double") {
                body << "  " << tmp << " = fsub double 0.0, " << val << "\n";
            } else {
                if (type != "i32") val = convert(val, type, "i32");
                type = "i32";
                body << "  " << tmp << " = sub i32 0, " << val << "\n";
            }
            outType = type;
            return tmp;
        }
        outType = type;
        return val;
    }
    if (auto* bin = dynamic_cast<BinaryExpression*>(expr)) {
        std::string lt, rt;
        std::string l = emitExpression(bin->left.get(), lt);
        std::string r = emitExpression(bin->right.get(), rt);

        // comparisons
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
            std::string cmpType = (lt == "double" || rt == "double") ? "double" : "i32";
            if (lt != cmpType) l = convert(l, lt, cmpType);
            if (rt != cmpType) r = convert(r, rt, cmpType);
            std::string tmp = nextTemp();
            if (cmpType == "double") {
                std::string op;
                if (bin->op == "==") op = "fcmp oeq";
                else if (bin->op == "!=") op = "fcmp one";
                else if (bin->op == "<") op = "fcmp olt";
                else if (bin->op == ">") op = "fcmp ogt";
                else if (bin->op == "<=") op = "fcmp ole";
                else op = "fcmp oge";
                body << "  " << tmp << " = " << op << " double " << l << ", " << r << "\n";
            } else {
                std::string op;
                if (bin->op == "==") op = "icmp eq";
                else if (bin->op == "!=") op = "icmp ne";
                else if (bin->op == "<") op = "icmp slt";
                else if (bin->op == ">") op = "icmp sgt";
                else if (bin->op == "<=") op = "icmp sle";
                else op = "icmp sge";
                body << "  " << tmp << " = " << op << " i32 " << l << ", " << r << "\n";
            }
            outType = "i1";
            return tmp;
        }

        std::string resType = (lt == "double" || rt == "double") ? "double" : "i32";
        if (lt != resType) l = convert(l, lt, resType);
        if (rt != resType) r = convert(r, rt, resType);
        std::string tmp = nextTemp();
        if (resType == "double") {
            std::string op = bin->op == "+" ? "fadd" : bin->op == "-" ? "fsub" : bin->op == "*" ? "fmul" : "fdiv";
            body << "  " << tmp << " = " << op << " double " << l << ", " << r << "\n";
        } else {
            std::string op = bin->op == "+" ? "add" : bin->op == "-" ? "sub" : bin->op == "*" ? "mul" : "sdiv";
            body << "  " << tmp << " = " << op << " i32 " << l << ", " << r << "\n";
        }
        outType = resType;
        return tmp;
    }
    if (auto* call = dynamic_cast<CallExpression*>(expr)) {
        // builtins
        if (call->name == "sqrt" && call->arguments.size() == 1) {
            std::string t;
            std::string v = emitExpression(call->arguments[0].get(), t);
            if (t != "double") v = convert(v, t, "double");
            std::string tmp = nextTemp();
            body << "  " << tmp << " = call double @sqrt(double " << v << ")\n";
            outType = "double";
            return tmp;
        }
        if (call->name == "rand" && call->arguments.size() == 2) {
            std::string tMin, tMax;
            std::string minv = emitExpression(call->arguments[0].get(), tMin);
            std::string maxv = emitExpression(call->arguments[1].get(), tMax);
            minv = convert(minv, tMin, "i32");
            maxv = convert(maxv, tMax, "i32");
            std::string seeded = nextTemp();
            std::string seedLabel = nextLabel("seed");
            std::string contLabel = nextLabel("cont");
            body << "  " << seeded << " = load i1, i1* @rand_seeded, align 1\n";
            body << "  br i1 " << seeded << ", label %" << contLabel << ", label %" << seedLabel << "\n";
            body << seedLabel << ":\n";
            std::string timeReg = nextTemp();
            std::string truncReg = nextTemp();
            body << "  " << timeReg << " = call i64 @time(i8* null)\n";
            body << "  " << truncReg << " = trunc i64 " << timeReg << " to i32\n";
            body << "  store i32 " << truncReg << ", i32* @rand_seed, align 4\n";
            body << "  store i1 true, i1* @rand_seeded, align 1\n";
            body << "  br label %" << contLabel << "\n";
            body << contLabel << ":\n";
            std::string seed = nextTemp();
            body << "  " << seed << " = load i32, i32* @rand_seed, align 4\n";
            std::string s1 = nextTemp(), s2 = nextTemp(), s3 = nextTemp();
            body << "  " << s1 << " = mul i32 " << seed << ", 1103515245\n";
            body << "  " << s2 << " = add i32 " << s1 << ", 12345\n";
            body << "  " << s3 << " = and i32 " << s2 << ", 2147483647\n";
            body << "  store i32 " << s3 << ", i32* @rand_seed, align 4\n";
            std::string range = nextTemp();
            std::string size = nextTemp();
            std::string scaled = nextTemp();
            std::string result = nextTemp();
            body << "  " << range << " = sub i32 " << maxv << ", " << minv << "\n";
            body << "  " << size << " = add i32 " << range << ", 1\n";
            body << "  " << scaled << " = urem i32 " << s3 << ", " << size << "\n";
            body << "  " << result << " = add i32 " << minv << ", " << scaled << "\n";
            outType = "i32";
            return result;
        }

        std::string key = call->ns.empty() ? call->name : call->ns + "." + call->name;
        auto it = functions.find(key);
        if (it == functions.end()) {
            outType = "i32";
            return "0";
        }
        const FunctionInfo& info = it->second;
        std::vector<std::string> argValues;
        std::vector<std::string> argTypes;
        for (size_t i = 0; i < call->arguments.size(); ++i) {
            std::string t;
            std::string v = emitExpression(call->arguments[i].get(), t);
            if (i < info.parameters.size()) {
                std::string expected = mapType(info.parameters[i].type);
                if (t != expected) v = convert(v, t, expected);
                t = expected;
            }
            argValues.push_back(v);
            argTypes.push_back(t);
        }
        std::string res;
        if (info.returnType != "void") {
            res = nextTemp();
            body << "  " << res << " = call " << info.returnType << " @" << info.irName << "(";
        } else {
            body << "  call void @" << info.irName << "(";
        }
        for (size_t i = 0; i < argValues.size(); ++i) {
            if (i > 0) body << ", ";
            body << argTypes[i] << " " << argValues[i];
        }
        body << ")\n";
        outType = info.returnType;
        return res;
    }

    if (auto* assign = dynamic_cast<AssignmentExpression*>(expr)) {
        VariableInfo* target = resolveVariable(assign->name);
        if (!target) {
            outType = "i32";
            return "0";
        }
        std::string rhsType;
        std::string rhs = emitExpression(assign->value.get(), rhsType);
        if (rhsType != target->type) rhs = convert(rhs, rhsType, target->type);
        int align = alignmentFor(target->type);
        body << "  store " << target->type << " " << rhs << ", " << target->type << "* " << target->address << ", align " << align << "\n";
        outType = target->type;
        return rhs;
    }

    outType = "i32";
    return "0";
}

std::string CodeGenerator::convert(const std::string& value, const std::string& from, const std::string& to) {
    if (from == to) return value;
    std::string tmp = nextTemp();
    if (from == "i32" && to == "double") {
        body << "  " << tmp << " = sitofp i32 " << value << " to double\n";
        return tmp;
    }
    if (from == "double" && to == "i32") {
        body << "  " << tmp << " = fptosi double " << value << " to i32\n";
        return tmp;
    }
    if (from == "i32" && to == "i1") {
        body << "  " << tmp << " = icmp ne i32 " << value << ", 0\n";
        return tmp;
    }
    if (from == "double" && to == "i1") {
        body << "  " << tmp << " = fcmp one double " << value << ", 0.0\n";
        return tmp;
    }
    if (from == "i1" && to == "i32") {
        body << "  " << tmp << " = zext i1 " << value << " to i32\n";
        return tmp;
    }
    if (from == "i1" && to == "double") {
        std::string mid = nextTemp();
        body << "  " << mid << " = zext i1 " << value << " to i32\n";
        body << "  " << tmp << " = sitofp i32 " << mid << " to double\n";
        return tmp;
    }
    // unknown conversion, return original
    return value;
}
