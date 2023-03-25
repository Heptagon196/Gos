#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Reflection/ReflMgr.h"

namespace GosVM {
    struct RTConstNum;
}

namespace std {
    template<> class hash<GosVM::RTConstNum> {
        public:
            size_t operator() (const GosVM::RTConstNum& num) const;
    };
}

namespace GosVM {
    enum VMOperator {
        NONE,

        DEF_CLASS,          // CLASS [class name]
        DEF_MEMBER_VAR,     // VAR [type] [name]
        DEF_MEMBER_FUNC,    // FUNC [name] [return type] [param count] {param type}

        STR,                // STR [str id] [str len] {str}
        NUM,                // NUM [val id] [int64]
        NEW,                // NEW [type] [var id]
        NEW_STR,            // NEW_STR [var id] [string]
        NEW_NUM,            // NEW_NUM [i8/i32/i64/f/d] [var id] [value]
        MOV,                // MOV [var id] [var id]
        ARG,                // ARG [var id] [int]
        GET_FIELD,          // GET_FIELD [var id] [var id] [name]

        ADD,                // MOV [var id] [var id]
        SUB,                // SUB [var id] [var id]
        MUL,                // MUL [var id] [var id]
        DIV,                // DIV [var id] [var id]
        REM,                // REM [var id] [var id]

        AND,                // AND [var id] [var id]
        OR,                 // OR [var id] [var id]
        XOR,                // XOR [var id] [var id]

        RET,                // RET [var id]
        JMP,                // JMP [int]
        IF,                 // IF [var id] [int]

        CALL,               // CALL [var id] [method name] [param count] {param var id}
        CALL_STATIC,        // CALL_STATIC [type name] [method name] [param count] {param var id}
    };
    class IRTokenizer {
        private:
            char last;
            std::istream& input;
        public:
            IRTokenizer(std::istream& input);
            std::string GetToken();
            template<typename T> T GetToken() {
                return TokenTo<T>(GetToken());
            }
            bool IsEOL() const;
            bool IsEOF() const;
            template<typename T> static T TokenTo(std::string token);
    };
    struct RTConstNum {
        union {
            int8_t i8;
            int32_t i32;
            int64_t i64;
            float f;
            double d;
        } data;
        bool operator == (const RTConstNum& other) const;
    };
    struct RTConst {
        std::vector<std::string> strs;
        std::unordered_map<std::string, int> strMap;
        int getStrID(const std::string& s);
        void setStrID(const std::string& s, int id);

        std::vector<RTConstNum> nums;
        std::unordered_map<RTConstNum, int> numMap;
        int getNumID(RTConstNum s);
        void setNumID(RTConstNum s, int id);
    };
    struct RTMemory {
        std::vector<SharedObject> mem;
        void NewVar(int id, SharedObject obj);
    };
    class VMExecutable {
        private:
            std::string* curContent;
        protected:
            int readProgress;
            int startPos, endPos;
            void StartRead();
            char& GetOperation();
            char& GetParamBool();
            char& GetParamByte();
            int& GetParamInt();
            int64_t& GetParamInt64();
            int64_t& GetParamAddr();
            float& GetParamFloat();
            double& GetParamDouble();
            std::string& GetParamString();
            int& GetParamClass();
        public:
            virtual RTMemory& getMem() = 0;
            virtual RTConst& getConst() = 0;
            virtual std::string& getContent() = 0;
            void Write(std::ostream& out, bool prettified = true);
            SharedObject Execute(ObjectPtr instance = {}, const std::vector<ObjectPtr>& params = {});
    };
    class VMProgram;
    class VMFunction : public VMExecutable {
        private:
            VMExecutable* program;
            RTMemory& getMem() override;
            RTConst& getConst() override;
            std::string& getContent() override;
        public:
            VMFunction();
            VMFunction(VMExecutable* program, int progress);
    };
    class VMProgram : public VMExecutable {
        private:
            friend VMFunction;
            std::string content;
            RTMemory mem;
            RTConst cst;
            RTMemory& getMem() override;
            RTConst& getConst() override;
            std::string& getContent() override;
            void AddOperation(char op);
            void AddParamBool(char param);
            void AddParamByte(char param);
            void AddParamInt(int param);
            void AddParamInt64(int64_t param);
            void AddParamAddr(int64_t param);
            void AddParamFloat(float param);
            void AddParamDouble(double param);
            void AddParamString(std::string param);
            void AddParamClass(int64_t param);
        public:
            VMProgram();
            VMProgram(std::string content);
            void Read(std::istream& input, bool prettified = true);
    };
    class GosClass {
        public:
            std::string className;
            static std::map<TypeID, GosClass> classInfo;
            std::vector<std::pair<TypeID, std::string>> vars;
            std::unordered_map<std::string, VMFunction> method;
            GosClass();
            GosClass(std::string className);
    };
    class GosInstance {
        public:
            GosClass* info;
            std::unordered_map<std::string, SharedObject> field;
    };
}
