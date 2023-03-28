#pragma once

#include "GosVM.h"
#include "GosTokenizer.h"

namespace Gos {
    class GosASTError {
        public:
            static void SetFileName(const std::string& name);
            static void UnexpectedToken(GosToken token);
            static void ErrorToken(GosToken token, GosToken correct);
    };
    namespace Compiler {
        struct Scope {
            Scope* fa;
            std::vector<Scope> subScopes;
            int* pos;
            std::unordered_map<std::string, int> varID;
            std::unordered_map<std::string, std::string> varType;
            int GetVarID(const std::string& name);
            std::string GetVarType(const std::string& name);
        };
        struct VMCompiler {
            static int lambdaCount;
            Scope mainScope;
            Scope* currentScope;
            int pos;
            GosVM::VMProgram& vm;
            std::unordered_map<int, bool> tmpVars;
            std::vector<int> lambdaDefStartPos;
            std::vector<std::function<void(int)>> lambdaDefJumpout;
            int GetTmpVar();
            void ClearTmp();
            void MoveToNew();
            void MoveBack();
            void SetProgramFileName(const std::string& name);
            VMCompiler(GosVM::VMProgram& program);
        };
    }
    class GosAST {
        public:
            int branch;
            std::vector<GosAST*> nodes;
            GosToken token;
            void Build(Gos::GosTokenizer& tokenizer);
            virtual int CompileAST(Compiler::VMCompiler& compiler);
            virtual void PrintAST(int indent = 0);
            virtual ~GosAST();
    };

#define Build(name) name(Gos::GosTokenizer& tokenizer)
#define Compile() CompileAST(Compiler::VMCompiler& code)
#define Print() PrintAST(int indent)
#define TokenList std::vector<GosToken> tokenList;
#define GOS_AST(name, extra)                                \
class name : public GosAST {                                \
    public:                                                 \
    Build(name);                                            \
    int Compile() override;                                 \
    void Print() override;                                  \
    extra                                                   \
}
    namespace AST {
        class Empty : public GosAST {
            public:
                Empty();
                int Compile() override;
                void Print() override;
        };
        GOS_AST(Symbol,);
        GOS_AST(Number,);
        GOS_AST(String,);
        GOS_AST(Primary,);
        GOS_AST(PostFix,);
        GOS_AST(Unary,);
        GOS_AST(Cast,);
        GOS_AST(Mul, TokenList);
        GOS_AST(Add, TokenList);
        GOS_AST(Relation, TokenList);
        GOS_AST(Equality, TokenList);
        GOS_AST(LogicAnd, TokenList);
        GOS_AST(LogicOr, TokenList);
        GOS_AST(Cond,);
        GOS_AST(Const,);
        GOS_AST(Assign,);
        GOS_AST(Exp,);
        GOS_AST(DefVar, TokenList);
        GOS_AST(Statement,);
        GOS_AST(StatBlock,);
        GOS_AST(DefArg,);
        GOS_AST(DefArgList,);
        GOS_AST(ArgsList,);
        GOS_AST(LambdaDef,);
        GOS_AST(ClassDef,);
        GOS_AST(IDList,);
        GOS_AST(SingleAttr,);
        GOS_AST(Attribute,);
        GOS_AST(Preprocess,);
    }
#undef Build
#undef Compile
#undef Print
#undef TokenList
#undef GOS_AST
};
