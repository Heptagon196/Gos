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

        DEF_CLASS,          // CLASS [class name] [inherit count] {inherits}
        ATTR,               // ATTR [attr name] [param count] {params}
        DEF_MEMBER_VAR,     // VAR [type] [name]
        DEF_MEMBER_FUNC,    // FUNC [name] [return type] [param count] {param type} [jump offset]
        DEF_STATIC_VAR,     // STATIC_VAR [type] [name]
        DEF_STATIC_FUNC,    // STATIC_FUNC [name] [return type] [param count] {param type} [jump offset]

        ALLOC,              // ALLOC [size]
        STR,                // STR [str id] [str len] {str}
        NUM,                // NUM [val id] [int64]
        NS,                 // NS [type]
        NEW,                // NEW [type] [var id] [param count] {params}
        NEW_STR,            // NEW_STR [var id] [string]
        NEW_NUM,            // NEW_NUM [i8/i32/i64/f/d] [var id] [value]
        MOV,                // MOV [var id] [var id]
        REF,                // REF [var id] [var id]
        CLONE,              // CLONE [var id] [var id]
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
        NOT,                // NOT [var id]

        RET,                // RET [var id]
        JMP,                // JMP [int]
        IF,                 // IF [var id] [int]

        BOX,                // BOX [var id] [var id]
        UNBOX,              // UNBOX [var id] [var id]

        CALL,               // CALL [var id] [method name] [param count] {param var id}
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
            int8_t i8;          // 0
            int32_t i32;        // 1
            int64_t i64;        // 2
            float f;            // 3
            double d;           // 4
        } data;
        bool operator == (const RTConstNum& other) const;
    };
    struct RTConst {
        std::vector<std::shared_ptr<std::string>> strs;
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
            int startPos, endPos;
            int readProgress;
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
            virtual SharedObject Execute(ObjectPtr instance = {}, const std::vector<ObjectPtr>& params = {});
    };
    class VMProgram;
    class VMFunction : public VMExecutable {
        private:
            VMExecutable* program;
            RTMemory* mem;
            RTMemory& getMem() override;
            RTConst& getConst() override;
            std::string& getContent() override;
        public:
            VMFunction();
            VMFunction(VMExecutable* program, int progress);
            SharedObject Execute(ObjectPtr instance = {}, const std::vector<ObjectPtr>& params = {}) override;
    };
    class VMProgram : public VMExecutable {
        private:
            friend VMFunction;
            std::string content;
            int readProgress;
            RTMemory mem;
            RTConst* cst;
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
            VMProgram(RTConst* constArea);
            void Read(std::istream& input, bool prettified = true);
            // returns a function to set jump offset
            void WriteCommandDefClass(const std::string& className, const std::vector<std::string>& inherits);
            void WriteCommandDefVar(const std::string& type, const std::string& varName);
            void WriteCommandNamespace(const std::string& type);
            std::function<void(int)> WriteCommandDefFunc(const std::string& name, const std::string& retType, const std::vector<std::string>& paramTypes);
            void WriteCommandDefStaticVar(const std::string& type, const std::string& varName);
            std::function<void(int)> WriteCommandDefStaticFunc(const std::string& name, const std::string& retType, const std::vector<std::string>& paramTypes);
            void WriteCommandNew(const std::string& type, int varID, const std::vector<int>& params);
            void WriteCommandNewStr(int varID, const std::string& val);
            void WriteCommandNewNum(int8_t type, int varID, RTConstNum num);
            void WriteCommandMov(int varA, int varB);
            void WriteCommandRef(int varA, int varB);
            void WriteCommandClone(int varA, int varB);
            void WriteCommandArg(int varID, int paramID);
            void WriteCommandGetField(int varID, int sourceVarID, const std::string& fieldName);
            void WriteCommandAdd(int varA, int varB);
            void WriteCommandSub(int varA, int varB);
            void WriteCommandMul(int varA, int varB);
            void WriteCommandDiv(int varA, int varB);
            void WriteCommandRem(int varA, int varB);
            void WriteCommandXor(int varA, int varB);
            void WriteCommandAnd(int varA, int varB);
            void WriteCommandOr(int varA, int varB);
            void WriteCommandNot(int varA);
            void WriteCommandRet(int varID);
            void WriteCommandAttributes(const std::string& attr, const std::vector<std::string>& params);
            std::function<void(int)> WriteCommandJmp();
            std::function<void(int)> WriteCommandIf(int varID);
            void WriteCommandBox(int varA, int varB);
            void WriteCommandUnbox(int varA, int varB);
            void WriteCommandCall(int varID, const std::string& func, std::vector<int> paramsID);
            std::function<void(int)> WriteAlloc();
            void WriteFinish();
            int GetProgress() const;
    };
    class GosClass {
        public:
            std::string className;
            static std::unordered_map<TypeID, GosClass> classInfo;
            static std::unordered_map<TypeID, std::unordered_map<std::string, SharedObject>> staticVars;
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
