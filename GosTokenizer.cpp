#include <sstream>
#include "GosTokenizer.h"

#define RESERVED_WORD_COUNT 14

const std::string Gos::GosToken::TokenName[] = {
    "import",
    "class",
    "var",
    "func",
    "lambda",
    "for",
    "foreach",
    "if",
    "else",
    "while",
    "return",
    "break",
    "continue",
    "as",
    "=",
    "+=",
    "-=",
    "*=",
    "/=",
    "%=",
    "^=",
    "&=",
    "|=",
    "++",
    "--",
    "+",
    "-",
    "*",
    "/",
    "%",
    "^",
    "==",
    "!=",
    "<",
    ">",
    "<=",
    ">=",
    "!",
    "&&",
    "||",

    "symbol",
    "number",
    "string",

    ",",
    ";",
    ".",
    ":",
    "?",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    "EOF",
};

Gos::GosToken::GosToken() {}
Gos::GosToken::GosToken(TokenType type, int line) : type(type), line(line) {}
Gos::GosToken::GosToken(TokenType type, const std::string& str, int line) : type(type), line(line), str(str) {}
Gos::GosToken::GosToken(TokenType type, int8_t numType, GosVM::RTConstNum num, int line) : type(type), line(line), numType(numType), num(num) {}

std::string Gos::GosToken::ToString() const {
    if (type == SYMBOL) {
        return str;
    } else if (type == NUMBER) {
        std::stringstream ss;
        ss << ((std::string[]){ "i8", "i32", "i64", "float", "double" })[numType] << " ";
        if (numType < 3) {
            ss << num.data.i64;
        } else if (numType == 3) {
            ss << num.data.f;
        } else if (numType == 4) {
            ss << num.data.d;
        }
        return ss.str();
    } else if (type == STRING) {
        return "\"" + str + "\"";
    }
    return TokenName[type];
}

Gos::GosTokenizer::GosTokenizer(std::istream& fin, const std::string& inputName) : fin(fin), lineCount(1), inputName(inputName), pos(0) {
    GosToken token;
    while ((token = ReadToken()).type != NONE) {
        tokens.push_back(token);
    }
}

static inline Gos::GosToken NullToken = { Gos::NONE, -1 };

const Gos::GosToken& Gos::GosTokenizer::GetToken() {
    if (pos >= tokens.size()) {
        return NullToken;
    }
    return tokens[pos++];
}

void Gos::GosTokenizer::SetErrorCallback(std::function<bool()> callback) {
    errorCallback = callback;
}

void Gos::GosTokenizer::EatToken(Gos::TokenType type) {
    auto recv = GetToken();
    if (recv.type != type) {
        if (errorCallback != 0) {
            if (!errorCallback()) {
                return;
            }
        }
        std::cerr << "Error: " << inputName << ": " << recv.line << ": invalid token: expected \"" << GosToken::TokenName[type] << "\" but \"" << recv.ToString() << "\" found" << std::endl;
    }
}

void Gos::GosTokenizer::BackToken() {
    pos--;
}

int Gos::GosTokenizer::GetProgress() {
    return pos;
}

void Gos::GosTokenizer::RestoreProgress(int progress) {
    pos = progress;
}

bool Gos::GosTokenizer::IsEOF() {
    return pos >= tokens.size() || (pos > 0 && tokens[pos - 1].type == NONE);
}

#define TRY_GET(a, b)               \
    ch = fin.get();                 \
    if (ch == '=') {                \
        return { a, lineCount };    \
    } else {                        \
        fin.unget();                \
        return { b, lineCount };    \
    }

Gos::GosToken Gos::GosTokenizer::ReadToken() {
    char ch = 0;
    while (!ch) {
        ch = fin.get();
    }
    while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
        if (ch == '\n') {
            lineCount++;
        }
        ch = fin.get();
    }
    if (ch == EOF) {
        return { NONE, lineCount };
    }
    if (isdigit(ch) || ch == '+' || ch == '-' || ch == '.') {
        std::stringstream ss;
        ss << ch;
        char start = ch;
        bool startWithSign = ch == '+' || ch == '-';
        bool hasDot = ch == '.';
        ch = fin.get();
        if (startWithSign && !isdigit(ch)) {
            if (start == '+') {
                if (ch == '=') {
                    return { ASSIGN_ADD, lineCount };
                } else if (ch == '+') {
                    return { INC, lineCount };
                } else {
                    fin.unget();
                    return { ADD, lineCount };
                }
            } else {
                if (ch == '=') {
                    return { ASSIGN_SUB, lineCount };
                } else if (ch == '-') {
                    return { DEC, lineCount };
                } else {
                    fin.unget();
                    return { SUB, lineCount };
                }
            }
        }
        if (hasDot && !isdigit(ch)) {
            fin.unget();
            return { DOT, lineCount };
        }
        while (isdigit(ch) || ch == '.') {
            if (ch == '.') {
                if (hasDot) {
                    std::cerr << "Error: " << inputName << ": " << lineCount << ": illegal number format" << std::endl;
                }
                hasDot = true;
            }
            ss << ch;
            ch = fin.get();
        }
        int8_t numType;
        GosVM::RTConstNum num;
        num.data.i64 = 0;
        if (hasDot) {
            if (ch == 'f') {
                ss >> num.data.f;
                numType = 3;
            } else {
                fin.unget();
                ss >> num.data.d;
                numType = 4;
            }
        } else {
            numType = 1;
            if (ch == 'i') {
                ss >> num.data.i64;
                ch = fin.get();
                if (ch == '8') {
                    numType = 0;
                } else if (ch == '3') {
                    fin.get();
                    numType = 1;
                } else if (ch == '6') {
                    fin.get();
                    numType = 2;
                }
            } else if (ch == 'l') {
                ss >> num.data.i64;
                numType = 2;
            } else if (ch == 'f') {
                ss >> num.data.f;
                numType = 3;
            } else if (ch == 'd') {
                ss >> num.data.d;
                numType = 4;
            } else {
                ss >> num.data.i64;
                fin.unget();
            }
        }
        return { NUMBER, numType, num, lineCount };
    } else if (ch == '*') {
        TRY_GET(ASSIGN_MUL, MUL);
    } else if (ch == '/') {
        TRY_GET(ASSIGN_DIV, DIV);
    } else if (ch == '%') {
        TRY_GET(ASSIGN_REM, REM);
    } else if (ch == '^') {
        TRY_GET(ASSIGN_XOR, XOR);
    } else if (ch == '=') {
        TRY_GET(E, ASSIGN);
    } else if (ch == '!') {
        TRY_GET(NE, NOT);
    } else if (ch == '<') {
        TRY_GET(LE, L);
    } else if (ch == '>') {
        TRY_GET(GE, G);
    } else if (ch == '&') {
        ch = fin.get();
        if (ch == '=') {
            return { ASSIGN_AND, lineCount };
        } else {
            if (ch != '&') {
                fin.unget();
            }
            return { AND, lineCount };
        }
    } else if (ch == '|') {
        ch = fin.get();
        if (ch == '=') {
            return { ASSIGN_OR, lineCount };
        } else {
            if (ch != '|') {
                fin.unget();
            }
            return { OR, lineCount };
        }
    } else if (ch == ',') {
        return { COM, lineCount };
    } else if (ch == ';') {
        return { SEM, lineCount };
    } else if (ch == ':') {
        return { COLON, lineCount };
    } else if (ch == '?') {
        return { QUESTION, lineCount };
    } else if (ch == '(') {
        return { L_ROUND, lineCount };
    } else if (ch == ')') {
        return { R_ROUND, lineCount };
    } else if (ch == '[') {
        return { L_SQUARE, lineCount };
    } else if (ch == ']') {
        return { R_SQUARE, lineCount };
    } else if (ch == '{') {
        return { L_CURLY, lineCount };
    } else if (ch == '}') {
        return { R_CURLY, lineCount };
    } else if (ch == '\"') {
        int line = lineCount;
        std::string content = "";
        bool convNext = false;
        ch = fin.get();
        while (convNext || ch != '\"') {
            if (convNext) {
                if (ch == 'n') {
                    content += '\n';
                } else if (ch == 't') {
                    content += '\t';
                } else {
                    if (ch == '\n') {
                        lineCount++;
                    }
                    content += ch;
                }
                convNext = false;
            } else {
                if (ch == '\\') {
                    convNext = true;
                } else {
                    if (ch == '\n') {
                        lineCount++;
                    }
                    content += ch;
                }
            }
            ch = fin.get();
        }
        return { STRING, content, line };
    } else {
        std::string symbol = "";
        symbol += ch;
        ch = fin.get();
        bool prevColon = false;
        while (isalnum(ch) || ch == ':' || ch == '_') {
            if (prevColon && ch != ':') {
                fin.unget();
                symbol.pop_back();
                break;
            }
            prevColon = ch == ':' && !prevColon;
            symbol += ch;
            ch = fin.get();
        }
        if (prevColon) {
            symbol.pop_back();
            fin.unget();
        }
        fin.unget();
        TokenType type = SYMBOL;
        for (int i = 0; i < RESERVED_WORD_COUNT; i++) {
            if (GosToken::TokenName[i] == symbol) {
                type = (TokenType)i;
                break;
            }
        }
        if (type == SYMBOL) {
            return { type, symbol, lineCount };
        } else {
            return { type, lineCount };
        }
    }
}
