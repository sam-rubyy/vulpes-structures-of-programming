#include "error_handler.hpp"
#include <sstream>

ErrorHandler::ErrorHandler(const std::string& sourceCode, const std::string& filename) 
    : filename(filename) {
    
    // Split source code into lines for context display
    std::stringstream ss(sourceCode);
    std::string line;
    while (std::getline(ss, line)) {
        sourceLines.push_back(line);
    }
}

void ErrorHandler::addError(ErrorSeverity severity, const SourceLocation& location, const std::string& message) {
    std::string context = getSourceLine(location.line);
    errors.emplace_back(severity, location, message, context);
}

void ErrorHandler::addError(ErrorSeverity severity, int line, int column, const std::string& message) {
    SourceLocation loc(line, column, filename);
    addError(severity, loc, message);
}

void ErrorHandler::error(const SourceLocation& location, const std::string& message) {
    addError(ErrorSeverity::ERROR, location, message);
}

void ErrorHandler::error(int line, int column, const std::string& message) {
    addError(ErrorSeverity::ERROR, line, column, message);
}

void ErrorHandler::warning(const SourceLocation& location, const std::string& message) {
    addError(ErrorSeverity::WARNING, location, message);
}

void ErrorHandler::fatal(const SourceLocation& location, const std::string& message) {
    addError(ErrorSeverity::FATAL, location, message);
}

bool ErrorHandler::hasErrors() const {
    for (const auto& error : errors) {
        if (error.severity == ErrorSeverity::ERROR || error.severity == ErrorSeverity::FATAL) {
            return true;
        }
    }
    return false;
}

bool ErrorHandler::hasFatal() const {
    for (const auto& error : errors) {
        if (error.severity == ErrorSeverity::FATAL) {
            return true;
        }
    }
    return false;
}

size_t ErrorHandler::getErrorCount() const {
    size_t count = 0;
    for (const auto& error : errors) {
        if (error.severity == ErrorSeverity::ERROR || error.severity == ErrorSeverity::FATAL) {
            count++;
        }
    }
    return count;
}

std::string ErrorHandler::getSourceLine(int lineNumber) const {
    if (lineNumber > 0 && lineNumber <= static_cast<int>(sourceLines.size())) {
        return sourceLines[lineNumber - 1]; // Convert to 0-based index
    }
    return "";
}

void ErrorHandler::printErrors() const {
    for (const auto& error : errors) {
        printError(error);
    }
}

void ErrorHandler::printError(const CompilerError& error) const {
    std::cerr << severityToString(error.severity);
    
    if (!filename.empty()) {
        std::cerr << " in " << filename;
    }
    
    std::cerr << " at line " << error.location.line 
              << ", column " << error.location.column 
              << ": " << error.message << std::endl;
    
    // Show the source line with context
    if (!error.context.empty()) {
        std::cerr << "  " << error.context << std::endl;
        
        // Show a caret pointing to the error location
        std::cerr << "  ";
        for (int i = 1; i < error.location.column; i++) {
            std::cerr << " ";
        }
        std::cerr << "^" << std::endl;
    }
    std::cerr << std::endl;
}

std::string ErrorHandler::severityToString(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::WARNING: return "Warning";
        case ErrorSeverity::ERROR: return "Error";
        case ErrorSeverity::FATAL: return "Fatal Error";
        default: return "Unknown";
    }
}