#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"
#include "error_handler.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("could not open " + path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) throw std::runtime_error("could not write " + path);
    file << content;
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        std::string input = "main.vlp";
        std::string output = "a.out";
        bool showLLVM = false;
        bool runExec = false;
        bool clean = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--show-llvm" || arg == "-ll") showLLVM = true;
            else if (arg == "--run" || arg == "-r" || arg == "run") runExec = true;
            else if (arg == "--clean" || arg == "-c") clean = true;
            else if (arg == "-o" && i + 1 < argc) {
                output = argv[++i];
            } else if (arg.size() > 4 && arg.substr(arg.size() - 4) == ".vlp") {
                input = arg;
            }
        }

        if (clean) {
            std::string stem = input.substr(0, input.find_last_of('.'));
            std::string llFile = stem + ".ll";
            std::remove(llFile.c_str());
            std::remove(output.c_str());
            std::remove("a.out");
            return 0;
        }

        std::string source = readFile(input);
        Lexer lexer(source);
        ErrorHandler handler(source, input);
        Parser parser(lexer.tokens, handler);
        auto program = parser.parseProgram();
        if (handler.hasErrors()) {
            handler.printErrors();
            return 1;
        }

        CodeGenerator generator;
        std::string ir = generator.generate(program);

        std::string stem = input.substr(0, input.find_last_of('.'));
        std::string llFile = stem + ".ll";
        writeFile(llFile, ir);

        if (showLLVM) {
            std::cout << ir << std::endl;
        }

        std::string objFile = stem + ".o";
        std::string cmd = "clang -o " + output + " " + llFile + " -lm";
        int result = std::system(cmd.c_str());
        if (result != 0) {
            std::string llcCmd = "llc -filetype=obj " + llFile + " -o " + objFile;
            result = std::system(llcCmd.c_str());
            if (result == 0) {
                cmd = "gcc -o " + output + " " + objFile + " -lm";
                result = std::system(cmd.c_str());
            }
        }
        if (result != 0) {
            std::cerr << "Compilation failed (clang/llc/gcc not available?).\n";
            return 1;
        }
        std::cout << "Executable created: " << output << std::endl;

        if (runExec) {
            std::string run = "./" + output;
            std::system(run.c_str());
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
