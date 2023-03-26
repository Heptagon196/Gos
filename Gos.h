#pragma once

#include "GosVM.h"
#include "GosAST.h"

namespace Gos {
    class GosScript {
        private:
            std::string scriptName;
            TypeID scriptNameClass;
            GosVM::VMProgram vm;
            static GosVM::RTConst constArea;
        public:
            GosScript();
            void SetScriptName(const std::string& name);
            void Read(std::istream& fin, std::string inputFileName = "");
            SharedObject Execute(ObjectPtr instance = {}, const std::vector<ObjectPtr>& params = {});
            const GosVM::GosClass& GetClassInfo() const;
            SharedObject CreateInstance() const;
    };
}
