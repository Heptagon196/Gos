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
    ast.PrintAST();
}

void Gos::GosScript::PrintIR(std::ostream& fout, bool prettify) {
    vm.Write(fout, prettify);
}
