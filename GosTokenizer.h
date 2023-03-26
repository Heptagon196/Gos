#pragma once

#include <stack>
#include "GosVM.h"

namespace Gos {
    enum TokenType {
        IMPORT,
        CLASS,
        VAR,
        FUNC,
        LAMBDA,
        FOR,
        FOREACH,
        IF,
        ELSE,
        WHILE,
        RETURN,
        BREAK,
        CONTINUE,
        ASSIGN,
        ASSIGN_ADD,
        ASSIGN_SUB,
        ASSIGN_MUL,
        ASSIGN_DIV,
        ASSIGN_REM,
        ASSIGN_XOR,
        ASSIGN_AND,
        ASSIGN_OR,
        INC,
        DEC,
        ADD,
        SUB,
        MUL,
        DIV,
        REM,
        XOR,
        E,
        NE,
        L,
        G,
        LE,
        GE,
        NOT,
        AND,
        OR,

        SYMBOL,
        NUMBER,
        STRING,

        COM,
        SEM,
        DOT,
        COLON,
        L_ROUND,
        R_ROUND,
        L_SQUARE,
        R_SQUARE,
        L_CURLY,
        R_CURLY,

        NONE,
    };
    struct GosToken {
        static const std::string TokenName[];
        TokenType type;
        int line;
        int8_t numType;
        GosVM::RTConstNum num;
        std::string str;
        GosToken();
        GosToken(TokenType type, int line);
        GosToken(TokenType type, const std::string& str, int line);
        GosToken(TokenType type, int8_t numType, GosVM::RTConstNum num, int line);
    };
    class GosTokenizer {
        private:
            std::istream& fin;
            int lineCount;
            std::string inputName;
            std::stack<GosToken> st;
        public:
            GosTokenizer(std::istream& fin, const std::string& inputName);
            GosToken GetToken();
            void BackToken(const GosToken& token);
            void EatToken(TokenType type);
    };
}
