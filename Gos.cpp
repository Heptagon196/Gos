#include "Gos.h"
#include "GosTokenizer.h"

GosVM::RTConst Gos::GosScript::constArea;

Gos::GosScript::GosScript() : vm(&constArea) {}

void Gos::GosScript::SetScriptName(const std::string& name) {
    scriptName = name;
    std::string className = "";
    for (int i = 0; i < name.length() && name[i] != '.'; i++) {
        className += name[i];
    }
    scriptNameClass = TypeID::getRaw(className);
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
    GosASTError::SetFileName(inputFileName);
    GosAST ast;
    ast.Build(tokenizer);
    ast.PrintAST();
}
