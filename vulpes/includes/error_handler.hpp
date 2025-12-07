#pragma once
#include <string>
#include <vector>
#include <iostream>

struct SourceLocation {
    int line;
    int column;
    std::string filename;
    
    SourceLocation(int l = 0, int c = 0, const std::string& f = "") 
        : line(l), column(c), filename(f) {}
};

enum class ErrorSeverity {
    WARNING,
    ERROR,
    FATAL
};

struct CompilerError {
    ErrorSeverity severity;
    SourceLocation location;
    std::string message;
    std::string context; // The line of code that caused the error
    
    CompilerError(ErrorSeverity sev, const SourceLocation& loc, const std::string& msg, const std::string& ctx = "")
        : severity(sev), location(loc), message(msg), context(ctx) {}
};

class ErrorHandler {
public:
    ErrorHandler(const std::string& sourceCode = "", const std::string& filename = "");
    
    // Add an error
    void addError(ErrorSeverity severity, const SourceLocation& location, const std::string& message);
    void addError(ErrorSeverity severity, int line, int column, const std::string& message);
    
    // Convenience methods
    void error(const SourceLocation& location, const std::string& message);
    void error(int line, int column, const std::string& message);
    void warning(const SourceLocation& location, const std::string& message);
    void fatal(const SourceLocation& location, const std::string& message);
    
    // Check if there are errors
    bool hasErrors() const;
    bool hasFatal() const;
    size_t getErrorCount() const;
    
    // Display all errors
    void printErrors() const;
    
    // Get a specific line from source for context
    std::string getSourceLine(int lineNumber) const;

private:
    std::vector<CompilerError> errors;
    std::vector<std::string> sourceLines;
    std::string filename;
    
    void printError(const CompilerError& error) const;
    std::string severityToString(ErrorSeverity severity) const;
};