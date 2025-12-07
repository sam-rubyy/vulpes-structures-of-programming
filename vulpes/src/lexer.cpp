#include "lexer.hpp"
#include <cctype>

namespace {
bool isIdentifierStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isIdentifierPart(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}
} // namespace

Lexer::Lexer(const std::string& source) {
    size_t i = 0;
    int line = 1;
    int col = 1;

    auto bump = [&](char c) {
        if (c == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
    };

    while (i < source.size()) {
        char c = source[i];

        if (std::isspace(static_cast<unsigned char>(c))) {
            bump(c);
            i++;
            continue;
        }

        // line comments //
        if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
            while (i < source.size() && source[i] != '\n') {
                bump(source[i]);
                i++;
            }
            continue;
        }

        // string literal
        if (c == '"') {
            size_t start = i + 1;
            int startCol = col;
            i++;
            col++;
            std::string value;
            while (i < source.size() && source[i] != '"') {
                if (source[i] == '\\' && i + 1 < source.size()) {
                    char esc = source[i + 1];
                    if (esc == 'n') value.push_back('\n');
                    else if (esc == 't') value.push_back('\t');
                    else value.push_back(esc);
                    bump(source[i]);
                    bump(source[i + 1]);
                    i += 2;
                } else {
                    value.push_back(source[i]);
                    bump(source[i]);
                    i++;
                }
            }
            if (i < source.size() && source[i] == '"') {
                i++;
                col++;
            }
            add(TokenType::String, value, line, startCol);
            continue;
        }

        // numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            int startCol = col;
            size_t start = i;
            bool hasDot = false;
            while (i < source.size() &&
                   (std::isdigit(static_cast<unsigned char>(source[i])) || source[i] == '.')) {
                if (source[i] == '.') {
                    if (hasDot) break;
                    if (i + 1 < source.size() && source[i + 1] == '.') break;
                    hasDot = true;
                }
                bump(source[i]);
                i++;
            }
            std::string num = source.substr(start, i - start);
            add(hasDot ? TokenType::Float : TokenType::Number, num, line, startCol);
            continue;
        }

        // identifiers/keywords
        if (isIdentifierStart(c)) {
            int startCol = col;
            size_t start = i;
            while (i < source.size() && isIdentifierPart(source[i])) {
                bump(source[i]);
                i++;
            }
            std::string word = source.substr(start, i - start);
            TokenType type = TokenType::Identifier;
            if (word == "var") type = TokenType::Var;
            else if (word == "const") type = TokenType::Const;
            else if (word == "fx") type = TokenType::Fx;
            else if (word == "if") type = TokenType::If;
            else if (word == "else") type = TokenType::Else;
            else if (word == "for") type = TokenType::For;
            else if (word == "in") type = TokenType::In;
            else if (word == "while") type = TokenType::While;
            else if (word == "return") type = TokenType::Return;
            else if (word == "print") type = TokenType::Print;
            else if (word == "gather") type = TokenType::Gather;
            else if (word == "mod") type = TokenType::Mod;
            else if (word == "true") type = TokenType::True;
            else if (word == "false") type = TokenType::False;
            add(type, word, line, startCol);
            continue;
        }

        // punctuation/operators
        switch (c) {
            case '+': add(TokenType::Plus, "+", line, col); bump(c); i++; break;
            case '-':
                if (i + 1 < source.size() && source[i + 1] == '>') {
                    add(TokenType::Arrow, "->", line, col);
                    bump(c); bump(source[i + 1]); i += 2;
                } else {
                    add(TokenType::Minus, "-", line, col);
                    bump(c); i++;
                }
                break;
            case '*': add(TokenType::Star, "*", line, col); bump(c); i++; break;
            case '/': add(TokenType::Slash, "/", line, col); bump(c); i++; break;
            case '(': add(TokenType::LeftParen, "(", line, col); bump(c); i++; break;
            case ')': add(TokenType::RightParen, ")", line, col); bump(c); i++; break;
            case '{': add(TokenType::LeftBrace, "{", line, col); bump(c); i++; break;
            case '}': add(TokenType::RightBrace, "}", line, col); bump(c); i++; break;
            case ',': add(TokenType::Comma, ",", line, col); bump(c); i++; break;
            case ';': add(TokenType::Semicolon, ";", line, col); bump(c); i++; break;
            case ':':
                if (i + 1 < source.size() && source[i + 1] == ':') {
                    add(TokenType::ColonColon, "::", line, col);
                    bump(c); bump(source[i + 1]); i += 2;
                } else {
                    add(TokenType::Colon, ":", line, col);
                    bump(c); i++;
                }
                break;
            case '.':
                if (i + 1 < source.size() && source[i + 1] == '.') {
                    add(TokenType::DotDot, "..", line, col);
                    bump(c); bump(source[i + 1]); i += 2;
                } else {
                    add(TokenType::Dot, ".", line, col);
                    bump(c); i++;
                }
                break;
            case '=':
                if (i + 1 < source.size() && source[i + 1] == '=') {
                    add(TokenType::Equals, "==", line, col);
                    bump(c); bump(source[i + 1]); i += 2;
                } else {
                    add(TokenType::Assign, "=", line, col);
                    bump(c); i++;
                }
                break;
            case '!':
                if (i + 1 < source.size() && source[i + 1] == '=') {
                    add(TokenType::NotEquals, "!=", line, col);
                    bump(c); bump(source[i + 1]); i += 2;
                } else {
                    add(TokenType::Unknown, "!", line, col);
                    bump(c); i++;
                }
                break;
            case '<':
                if (i + 1 < source.size() && source[i + 1] == '=') {
                    add(TokenType::LessEq, "<=", line, col);
                    bump(c); bump(source[i + 1]); i += 2;
                } else {
                    add(TokenType::Less, "<", line, col);
                    bump(c); i++;
                }
                break;
            case '>':
                if (i + 1 < source.size() && source[i + 1] == '=') {
                    add(TokenType::GreaterEq, ">=", line, col);
                    bump(c); bump(source[i + 1]); i += 2;
                } else {
                    add(TokenType::Greater, ">", line, col);
                    bump(c); i++;
                }
                break;
            default:
                add(TokenType::Unknown, std::string(1, c), line, col);
                bump(c);
                i++;
                break;
        }
    }

    add(TokenType::EndOfFile, "", line, col);
}

void Lexer::add(TokenType type, const std::string& lexeme, int line, int column) {
    tokens.push_back({type, lexeme, line, column});
}
