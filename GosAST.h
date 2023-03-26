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
    class GosAST {
        protected:
            std::vector<GosAST*> nodes;
            GosToken token;
        public:
            void Build(Gos::GosTokenizer& tokenizer);
            virtual void CompileToVM(GosVM::VMProgram& program);
            virtual void PrintAST(int indent = 0);
            virtual ~GosAST();
    };
};
