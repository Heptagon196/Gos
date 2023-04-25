#include "Gos.h"
#include "GosTokenizer.h"

GosVM::RTConst Gos::GosScript::constArea;

Gos::GosScript::GosScript() : vm(&constArea) {}

void Gos::GosScript::SetScriptName(const std::string& name) {
    GosASTError::SetFileName(name);
    scriptName = name;
    int pos = name.length() - 1;
    while (name[pos] != '.' && pos > 0) {
        pos--;
    }
    if (pos == 0) {
        pos = name.length();
    }
    scriptNameClass = TypeID::getRaw(std::string_view(scriptName).substr(0, pos));
}

SharedObject Gos::GosScript::Execute(ObjectPtr instance, const std::vector<ObjectPtr>& params) {
    return vm.Execute(instance, params);
}

const GosVM::GosClass& Gos::GosScript::GetClassInfo() const {
    auto& info = GosVM::GosClass::classInfo;
    if (info.find(scriptNameClass) == info.end()) {
        std::cerr << "Error: no such class defined in script " << scriptName << ": " << scriptNameClass.getName() << std::endl;
        return info[TypeID::get<void>()];
    }
    return info[scriptNameClass];
}

SharedObject Gos::GosScript::CreateInstance() const {
    return ReflMgr::Instance().New(scriptNameClass);
}

void Gos::GosScript::Read(std::istream& fin, std::string inputFileName) {
    GosTokenizer tokenizer(fin, inputFileName);
    SetScriptName(inputFileName);
    Compiler::VMCompiler compiler(vm);
    GosAST ast;
    ast.Build(tokenizer);
    ast.CompileAST(compiler);
    // ast.PrintAST();
}

void Gos::GosScript::PrintIR(std::ostream& fout, bool prettify) {
    vm.Write(fout, prettify);
}

void Gos::GosProject::PreprocessFile(std::string scriptPath, std::queue<std::string>& imports) {
    scriptPath = Path(scriptPath).lexically_relative(".");
    if (vis.count(scriptPath) > 0) {
        return;
    }
    auto fin = FileSystem::Read(scriptPath);
    vis.insert(scriptPath);
    std::string line;
    Path dir = Path(scriptPath).lexically_relative(".").parent_path();
    while (std::getline(fin, line)) {
        if (line.starts_with("#import ")) {
            Path cur = dir;
            cur.append(line.substr(8, line.length() - 8));
            std::string nxtPath = cur.lexically_relative(".");
            PreprocessFile(nxtPath, imports);
        }
    }
    imports.push(scriptPath);
}

void Gos::GosProject::ExecuteFiles(std::queue<std::string>& files) {
    while (!files.empty()) {
        std::string scriptPath = files.front();
        std::cout << "Gos: Import " << scriptPath << std::endl;
        files.pop();
        auto fin = FileSystem::Read(scriptPath);
        GosScript& script = scripts[scriptPath];
        script.Read(fin, scriptPath);
        script.Execute();
    }
}

void Gos::GosProject::ScanDirectory(std::string directory, std::function<bool(std::string)> filter) {
    std::queue<std::string> imports;
    FileSystem::Iterate(".", std::function([&filter, &imports, this](Path path) {
        if (filter(path)) {
            PreprocessFile(path, imports);
        }
    }));
    ExecuteFiles(imports);
}

void Gos::GosProject::AddScript(std::string scriptPath) {
    std::queue<std::string> imports;
    PreprocessFile(scriptPath, imports);
    ExecuteFiles(imports);
}
