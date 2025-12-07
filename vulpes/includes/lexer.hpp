#pragma once
#include <string>
#include <vector>

enum class TokenType {
    Identifier,
    Number,
    Float,
    String,
    True,
    False,
    // keywords
    Var,
    Const,
    Fx,
    If,
    Else,
    For,
    In,
    While,
    Return,
    Print,
    Gather,
    Mod,
    // punctuation/operators
    Arrow,
    Colon,
    ColonColon,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Semicolon,
    Dot,
    DotDot,
    Plus,
    Minus,
    Star,
    Slash,
    Assign,
    Equals,
    NotEquals,
    Less,
    LessEq,
    Greater,
    GreaterEq,
    EndOfFile,
    Unknown
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokens;
private:
    void add(TokenType type, const std::string& lexeme, int line, int column);
};
