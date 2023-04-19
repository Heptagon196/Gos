#pragma once

#include "GosVM.h"
#include "GosAST.h"
#include <set>

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
            void PrintIR(std::ostream& fout, bool prettify = true);
            SharedObject Execute(ObjectPtr instance = {}, const std::vector<ObjectPtr>& params = {});
            const GosVM::GosClass& GetClassInfo() const;
            SharedObject CreateInstance() const;
    };
    class GosProject {
        private:
            std::unordered_map<std::string, GosScript> scripts;
            std::set<std::string> vis;
            void PreprocessFile(std::string scriptPath, std::queue<std::string>& imports);
            void ExecuteFiles(std::queue<std::string>& files);
        public:
            void ScanDirectory(std::string directory = ".", std::function<bool(std::string)> filter = std::function(
                [](std::string fileName) {
                    return fileName.size() > 4 && fileName.substr(fileName.size() - 4, 4) == ".gos";
                }));
            void AddScript(std::string scriptPath);
    };
}
