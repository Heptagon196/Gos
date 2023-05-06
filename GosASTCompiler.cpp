#include "GosAST.h"
#include <sstream>

static inline std::string compilingFileName;
static inline int compilingLine;
static inline std::string compilingClassName;
static inline std::vector<std::string> compilingClassVars;
static inline std::vector<std::string> compilingClassVarTypes;
static inline std::unordered_map<std::string, int> compilingClassVarID;

int Gos::Compiler::Scope::GetVarID(const std::string& name, std::string& errorInfo) {
    if (varID.find(name) != varID.end()) {
        return varID[name];
    }
    if (fa != nullptr) {
        return fa->GetVarID(name, errorInfo);
    }
    std::stringstream ss;
    ss << "Error: " << compilingFileName << ": " << compilingLine << ": no such variable: " << name;
    errorInfo = ss.str();
    return 0;
}

std::string Gos::Compiler::Scope::GetVarType(const std::string& name) {
    if (varType.find(name) != varType.end()) {
        return varType[name];
    }
    if (fa != nullptr) {
        return fa->GetVarType(name);
    }
    std::cerr << "Error: " << compilingFileName << ": " << compilingLine << ": unknown variable type of " << name << std::endl;
    return "unknown";
}

void Gos::Compiler::VMCompiler::MoveToNew() {
    currentScope->subScopes.push_back(Scope());
    Scope* newScope = &currentScope->subScopes[currentScope->subScopes.size() - 1];
    newScope->pos = currentScope->pos;
    newScope->fa = currentScope;
    currentScope = newScope;
}

void Gos::Compiler::VMCompiler::MoveBack() {
    currentScope = currentScope->fa;
}

int Gos::Compiler::VMCompiler::GetTmpVar() {
    for (auto& [i, b] : tmpVars) {
        if (!b) {
            b = true;
            return i;
        }
    }
    return pos++;
}

void Gos::Compiler::VMCompiler::ClearTmp() {
    for (auto& var : tmpVars) {
        var.second = false;
    }
}

void Gos::Compiler::VMCompiler::SetProgramFileName(const std::string& name) {
    compilingFileName = name;
}

Gos::Compiler::VMCompiler::VMCompiler(GosVM::VMProgram& program) : pos(1), vm(program) {
    mainScope.fa = nullptr;
    mainScope.pos = &pos;
    currentScope = &mainScope;
}

#define Compile(name) int Gos::AST::name::CompileAST(Gos::Compiler::VMCompiler& code)
#define SUB(id) nodes[id]->CompileAST(code)
#define vm code.vm
#define pc code.pos
#define scope code.currentScope

#define START()                     \
    compilingLine = token.line;     \
    if (nodes.size() == 0) {        \
        return 0;                   \
    }

// Compile Functions

Compile(Empty) { return 0; }

Compile(Symbol) { 
    compilingLine = token.line;
    if (token.str == "true" || token.str == "false") {
        int num = code.GetTmpVar();
        int tmp = code.GetTmpVar();
        GosVM::RTConstNum val;
        val.data.i32 = token.str == "true" ? 1 : 0;
        vm.WriteCommandNewNum(1, num, val);
        vm.WriteCommandNew("bool", tmp, { num });
        return tmp;
    }
    std::string errorInfo = "";
    int ret = code.currentScope->GetVarID(token.str, errorInfo);
    if (errorInfo.size() > 0) {
        if (compilingClassVarID.find(token.str) != compilingClassVarID.end()) {
            return compilingClassVarID[token.str];
        }
        for (std::string& s : compilingClassVars) {
            if (s == token.str) {
                int tmp = code.GetTmpVar();
                vm.WriteCommandGetField(tmp, 1, token.str);
                compilingClassVarID[token.str] = tmp;
                return tmp;
            }
        }
        std::cerr << errorInfo << std::endl;
    }
    return ret;
}

Compile(Number) {
    compilingLine = token.line;
    int id = pc++;
    vm.WriteCommandNewNum(token.numType, id, token.num);
    return id;
}

Compile(String) {
    compilingLine = token.line;
    int id = pc++;
    vm.WriteCommandNewStr(id, token.str);
    return id;
}

Compile(Primary) {
    START();
    if (branch & 1) {
        std::vector<int> params;
        if (branch & 2) {
            GosAST* argsList = nodes[1];
            for (auto* expr : argsList->nodes) {
                params.push_back(expr->CompileAST(code));
            }
        }
        int id = code.GetTmpVar();
        vm.WriteCommandNew(nodes[0]->token.str, id, params);
        return id;
    }
    return SUB(0);
}

Compile(PostFix) {
    START();
    int prv = SUB(0);
    std::vector<int> params;
    for (int i = 1; i < nodes.size(); i++) {
        GosAST* rt = nodes[i];
        int tmp = code.GetTmpVar();
        params.clear();
        switch (rt->branch) {
            case 1:
                vm.WriteCommandCall(prv, "__index", { rt->SUB(0) });
                vm.WriteCommandRef(tmp, 0);
                break;
            case 2:
                if (rt->nodes.size() > 0) {
                    GosAST* argsList = rt->nodes[0];
                    for (auto* expr : argsList->nodes) {
                        params.push_back(expr->CompileAST(code));
                    }
                }
                vm.WriteCommandCall(prv, "__call", params);
                vm.WriteCommandRef(tmp, 0);
                break;
            case 3:
                vm.WriteCommandGetField(tmp, prv, rt->nodes[0]->token.str);
                break;
            case 4:
                if (rt->nodes.size() > 1) {
                    GosAST* argsList = rt->nodes[1];
                    for (auto* expr : argsList->nodes) {
                        params.push_back(expr->CompileAST(code));
                    }
                }
                vm.WriteCommandCall(prv, rt->nodes[0]->token.str, params);
                vm.WriteCommandRef(tmp, 0);
                break;
            case 5:
                vm.WriteCommandCall(prv, "__dec", {});
                vm.WriteCommandRef(tmp, 0);
                break;
            case 6:
                vm.WriteCommandCall(prv, "__inc", {});
                vm.WriteCommandRef(tmp, 0);
                break;
        }
        prv = tmp;
    }
    return prv;
}

Compile(Unary) {
    START();
    if (branch == 1) {
        if (token.type == ADD) {
            return SUB(0);
        } else if (token.type == SUB) {
            int tmp = code.GetTmpVar();
            vm.WriteCommandCall(SUB(0), "__unm", {});
            vm.WriteCommandRef(tmp, 0);
            return tmp;
        } else if (token.type == NOT) {
            int tmp = code.GetTmpVar();
            vm.WriteCommandClone(tmp, SUB(0));
            vm.WriteCommandNot(tmp);
            return tmp;
        } else if (token.type == MUL) {
            // unbox
            int tmp = code.GetTmpVar();
            vm.WriteCommandUnbox(tmp, SUB(0));
            return tmp;
        } else if (token.type == ADDR) {
            // box
            int tmp = code.GetTmpVar();
            vm.WriteCommandBox(tmp, SUB(0));
            return tmp;
        }
    }
    return SUB(0);
}

Compile(Cast) {
    START();
    if (token.type == AS) {
        int tmp = code.GetTmpVar();
        vm.WriteCommandNew(nodes[1]->token.str, tmp, {});
        vm.WriteCommandMov(tmp, SUB(0));
        return tmp;
    }
    return SUB(0);
}

Compile(Mul) {
    START();
    if (tokenList.size() == 0) {
        return SUB(0);
    }
    int tmp = code.GetTmpVar();
    vm.WriteCommandClone(tmp, SUB(0));
    for (int i = 0; i < tokenList.size(); i++) {
        if (tokenList[i].type == MUL) {
            vm.WriteCommandMul(tmp, SUB(i + 1));
        } else if (tokenList[i].type == DIV) {
            vm.WriteCommandDiv(tmp, SUB(i + 1));
        } else if (tokenList[i].type == REM) {
            vm.WriteCommandRem(tmp, SUB(i + 1));
        }
    }
    return tmp;
}

Compile(Add) {
    START();
    if (tokenList.size() == 0) {
        return SUB(0);
    }
    int tmp = code.GetTmpVar();
    vm.WriteCommandClone(tmp, SUB(0));
    for (int i = 0; i < tokenList.size(); i++) {
        if (tokenList[i].type == ADD) {
            vm.WriteCommandAdd(tmp, SUB(i + 1));
        } else if (tokenList[i].type == SUB) {
            vm.WriteCommandSub(tmp, SUB(i + 1));
        }
    }
    return tmp;
}

Compile(Relation) {
    START();
    if (tokenList.size() == 0) {
        return SUB(0);
    }
    std::vector<int> bools;
    int tmp = code.GetTmpVar();
    GosVM::RTConstNum num;
    num.data.i64 = 1;
    vm.WriteCommandNew("bool", tmp, {});
    vm.WriteCommandNot(tmp);
    bools.push_back(SUB(0));
    for (int i = 0; i < tokenList.size(); i++) {
        bools.push_back(SUB(i + 1));
        const char* op = "__eq";
        if (tokenList[i].type == L) {
            op = "__lt";
        } else if (tokenList[i].type == LE) {
            op = "__le";
        } else if (tokenList[i].type == G) {
            op = "__gt";
        } else if (tokenList[i].type == GE) {
            op = "__ge";
        }
        vm.WriteCommandCall(bools[i], op, { bools[i + 1] });
        vm.WriteCommandAnd(tmp, 0);
    }
    return tmp;
}

Compile(Equality) {
    START();
    if (tokenList.size() == 0) {
        return SUB(0);
    }
    std::vector<int> bools;
    int tmp = code.GetTmpVar();
    GosVM::RTConstNum num;
    num.data.i64 = 1;
    vm.WriteCommandNew("bool", tmp, {});
    vm.WriteCommandNot(tmp);
    bools.push_back(SUB(0));
    for (int i = 0; i < tokenList.size(); i++) {
        bools.push_back(SUB(i + 1));
        const char* op = "__eq";
        if (tokenList[i].type == NE) {
            op = "__ne";
        }
        vm.WriteCommandCall(bools[i], op, { bools[i + 1] });
        vm.WriteCommandAnd(tmp, 0);
    }
    return tmp;
}

Compile(LogicAnd) {
    START();
    if (tokenList.size() == 0) {
        return SUB(0);
    }
    int tmp = code.GetTmpVar();
    vm.WriteCommandClone(tmp, SUB(0));
    for (int i = 0; i < tokenList.size(); i++) {
        vm.WriteCommandAnd(tmp, SUB(i + 1));
    }
    return tmp;
}

Compile(LogicOr) {
    START();
    if (tokenList.size() == 0) {
        return SUB(0);
    }
    int tmp = code.GetTmpVar();
    vm.WriteCommandClone(tmp, SUB(0));
    for (int i = 0; i < tokenList.size(); i++) {
        vm.WriteCommandOr(tmp, SUB(i + 1));
    }
    return tmp;
}

Compile(Cond) {
    START();
    if (branch == 1) {
        int tmp = code.GetTmpVar();
        int val = SUB(0);
        auto ifJmp = vm.WriteCommandIf(val);
        vm.WriteCommandClone(tmp, SUB(2));
        auto outJmp = vm.WriteCommandJmp();
        ifJmp(vm.GetProgress());
        vm.WriteCommandClone(tmp, SUB(1));
        outJmp(vm.GetProgress());
        return tmp;
    }
    return SUB(0);
}

Compile(Const) {
    START();
    return SUB(0);
}

Compile(Assign) {
    START();
    int dest = SUB(0);
    int src = SUB(1);
    switch (token.type) {
        case ASSIGN:     vm.WriteCommandMov(dest, src); break;
        case ASSIGN_ADD: vm.WriteCommandAdd(dest, src); break;
        case ASSIGN_SUB: vm.WriteCommandSub(dest, src); break;
        case ASSIGN_MUL: vm.WriteCommandMul(dest, src); break;
        case ASSIGN_DIV: vm.WriteCommandDiv(dest, src); break;
        case ASSIGN_REM: vm.WriteCommandRem(dest, src); break;
        case ASSIGN_XOR: vm.WriteCommandXor(dest, src); break;
        case ASSIGN_AND: vm.WriteCommandAnd(dest, src); break;
        case ASSIGN_OR:  vm.WriteCommandOr(dest, src);  break;
        default: return dest;
    }
    return dest;
}

Compile(Exp) {
    START();
    return SUB(0);
}

Compile(DefVar) {
    START();
    std::string symbol = nodes[0]->token.str;
    std::string type = "unknown";
    int assign;
    int id = pc++;
    code.currentScope->varID[symbol] = id;
    if (branch == 1) {
        type = nodes[1]->token.str;
    } else if (branch == 2) {
        assign = SUB(1);
    } else if (branch == 3) {
        type = nodes[1]->token.str;
        assign = SUB(2);
    }
    code.currentScope->varType[symbol] = type;
    if (branch <= 1) {
        vm.WriteCommandNew(type, id, {});
    } else if (branch == 2) {
        vm.WriteCommandRef(id, assign);
    } else if (branch == 3) {
        vm.WriteCommandNew(type, id, {});
        vm.WriteCommandMov(id, assign);
    }
    return id;
}

Compile(Statement) {
    static std::stack<std::function<void(int)>> loopStart;
    static std::stack<std::function<void(int)>> loopOut;
    START();
    int val, startPos, iterStart;
    std::function<void(int)> ifJmp, endJmp;
    switch (branch) {
        case 1: case 10: return SUB(0);
        case 7:
            // Return
            vm.WriteCommandRet(SUB(0));
            return 0;
        case 8:
            // Break
            loopOut.push(vm.WriteCommandJmp());
            return 0;
        case 9:
            // Continue
            loopStart.push(vm.WriteCommandJmp());
            return 0;
        case 2: case 3:
            // If
            val = SUB(0);
            code.MoveToNew();
            ifJmp = vm.WriteCommandIf(val);
            if (branch == 3) {
                SUB(2);
            }
            endJmp = vm.WriteCommandJmp();
            ifJmp(vm.GetProgress());
            SUB(1);
            endJmp(vm.GetProgress());
            code.MoveBack();
            return 0;
        case 4:
            // While
            startPos = vm.GetProgress();
            val = SUB(0);
            code.MoveToNew();
            ifJmp = vm.WriteCommandIf(val);
            endJmp = vm.WriteCommandJmp();
            ifJmp(vm.GetProgress());
            SUB(1);
            vm.WriteCommandJmp()(startPos);
            endJmp(vm.GetProgress());
            code.MoveBack();
            // Flow Control
            while (!loopStart.empty()) {
                loopStart.top()(startPos);
                loopStart.pop();
            }
            while (!loopOut.empty()) {
                loopOut.top()(vm.GetProgress());
                loopOut.pop();
            }
            return 0;
        case 5:
            // For
            // init
            SUB(0);
            startPos = vm.GetProgress();
            // while
            val = SUB(1);
            code.MoveToNew();
            ifJmp = vm.WriteCommandIf(val);
            endJmp = vm.WriteCommandJmp();
            ifJmp(vm.GetProgress());
            // statment
            SUB(3);
            // iter
            iterStart = vm.GetProgress();
            SUB(2);
            vm.WriteCommandJmp()(startPos);
            endJmp(vm.GetProgress());
            code.MoveBack();
            // Flow Control
            while (!loopStart.empty()) {
                loopStart.top()(iterStart);
                loopStart.pop();
            }
            while (!loopOut.empty()) {
                loopOut.top()(vm.GetProgress());
                loopOut.pop();
            }
            return 0;
        case 6:
            // Foreach
            code.MoveToNew();
            // init
            int iter = code.GetTmpVar();
            int var = pc++;
            code.currentScope->varID[nodes[0]->token.str] = var;
            code.currentScope->varType[nodes[0]->token.str] = "unknown";
            int expr = SUB(1);
            int iterEnd = code.GetTmpVar();
            vm.WriteCommandCall(expr, "__end", {});
            vm.WriteCommandRef(iterEnd, 0);
            vm.WriteCommandCall(expr, "__begin", {});
            vm.WriteCommandRef(iter, 0);
            vm.WriteCommandCall(iter, "__indirection", {});
            vm.WriteCommandRef(var, 0);
            startPos = vm.GetProgress();
            // while
            vm.WriteCommandCall(iter, "__ne", { iterEnd });
            ifJmp = vm.WriteCommandIf(0);
            endJmp = vm.WriteCommandJmp();
            ifJmp(vm.GetProgress());
            // statment
            SUB(2);
            // iter
            iterStart = vm.GetProgress();
            vm.WriteCommandCall(iter, "__inc", {});
            vm.WriteCommandRef(iter, 0);
            vm.WriteCommandCall(iter, "__indirection", {});
            vm.WriteCommandRef(var, 0);
            vm.WriteCommandJmp()(startPos);
            endJmp(vm.GetProgress());
            code.MoveBack();
            // Flow Control
            while (!loopStart.empty()) {
                loopStart.top()(iterStart);
                loopStart.pop();
            }
            while (!loopOut.empty()) {
                loopOut.top()(vm.GetProgress());
                loopOut.pop();
            }
            return 0;
    }
    return 0;
}

static inline int blockDepth = 0;
Compile(StatBlock) {
    compilingLine = token.line;
    blockDepth++;
    for (auto* ast : nodes) {
        ast->CompileAST(code);
    }
    blockDepth--;
    return 0;
}

static inline int lambdaCount = 0;

Compile(LambdaDef) {
    START();

    int start = 0;
    std::stringstream ss;
    ss << compilingFileName << ":" << compilingLine << "_#" << (lambdaCount++);
    std::string lambdaName = ss.str();
    std::vector<std::pair<std::string, int>> captureVarNames;
    std::vector<int> captureType;
    std::vector<std::string> captureTypeName;
    std::vector<int> capturedID;
    std::vector<std::string> argTypes;
    std::vector<std::string> argNames;
    std::vector<std::string> ctorTypes;
    if (branch & 1) {
        std::string errorInfo;
        for (GosAST* ast : nodes[start]->nodes) {
            int id = code.currentScope->GetVarID(ast->token.str, errorInfo);
            std::string varType = "";
            if (errorInfo.size() > 0) {
                for (int i = 0; i < compilingClassVars.size(); i++) {
                    if (compilingClassVars[i] == ast->token.str) {
                        if (compilingClassVarID.find(ast->token.str) != compilingClassVarID.end()) {
                            id = compilingClassVarID[ast->token.str];
                        } else {
                            id = code.GetTmpVar();
                            vm.WriteCommandGetField(id, 1, ast->token.str);
                            compilingClassVarID[ast->token.str] = id;
                        }
                        varType = compilingClassVarTypes[i];
                        errorInfo = "";
                        break;
                    }
                }
                if (errorInfo.size() > 0) {
                    std::cerr << errorInfo << std::endl;
                }
            } else {
                varType = code.currentScope->GetVarType(ast->token.str);
            }
            captureVarNames.push_back({ ast->token.str, id });
            captureType.push_back(ast->branch);
            capturedID.push_back(captureVarNames[captureVarNames.size() - 1].second);
            captureTypeName.push_back(varType);
        }
        start++;
    }
    if (branch & 2) {
        for (auto* ast : nodes[start]->nodes) {
            argNames.push_back(ast->nodes[0]->token.str);
            argTypes.push_back(ast->nodes[1]->token.str);
        }
        start++;
    }

    auto jmpFromStartTo = vm.WriteCommandJmp();
    int startPos = vm.GetProgress();

    vm.WriteCommandDefClass(lambdaName, {});

    for (int i = 0; i < captureVarNames.size(); i++) {
        vm.WriteCommandDefVar(captureTypeName[i], captureVarNames[i].first);
        ctorTypes.push_back(captureTypeName[i]);
    }

    code.MoveToNew();
    int pcBack = pc;

    auto jmpCall = vm.WriteCommandDefFunc("__call", nodes[start++]->token.str, argTypes);
    auto setMemSize = vm.WriteAlloc();

    pc = 1;

    vm.WriteCommandArg(pc++, 0);
    for (int i = 0; i < argNames.size(); i++) {
        int id = pc++;
        vm.WriteCommandArg(id, i + 1);
        code.currentScope->varID[argNames[i]] = id;
        code.currentScope->varType[argNames[i]] = argTypes[i];
    }

    for (int i = 0; i < captureVarNames.size(); i++) {
        int id = pc++;
        vm.WriteCommandGetField(id, 1, captureVarNames[i].first);
        code.currentScope->varID[captureVarNames[i].first] = id;
        code.currentScope->varType[captureVarNames[i].first] = captureTypeName[i];
    }

    SUB(start++);

    jmpCall(vm.GetProgress());

    setMemSize(pc);
    pc = pcBack;
    code.MoveBack();

    code.lambdaDefStartPos.push_back(startPos);
    code.lambdaDefJumpout.push_back(vm.WriteCommandJmp());
    jmpFromStartTo(vm.GetProgress());

    int tmp = code.GetTmpVar();
    vm.WriteCommandNew(lambdaName, tmp, capturedID);

    for (int i = 0; i < captureVarNames.size(); i++) {
        int id = pc++;
        vm.WriteCommandGetField(id, tmp, captureVarNames[i].first);
        if (captureType[i] == 0) {
            if (captureTypeName[i] == "unknown") {
                vm.WriteCommandClone(id, captureVarNames[i].second);
            } else {
                vm.WriteCommandMov(id, captureVarNames[i].second);
            }
        } else {
            vm.WriteCommandRef(id, captureVarNames[i].second);
        }
    }

    return tmp;
}

Compile(DefArg) {
    START();
    return 0;
}

Compile(DefArgList) {
    START();
    return 0;
}

Compile(ArgsList) {
    START();
    return 0;
}

Compile(ClassDef) {
    START();
    int start = 0;
    if (branch & 1) {
        SUB(start++);
    }
    if (branch & 2) {
        // Func
        code.MoveToNew();
        compilingClassVarID.clear();
        std::string name = nodes[start++]->token.str;
        std::vector<std::string> argTypes;
        std::vector<std::string> argNames;
        if (branch & 8) {
            // DefArgList
            auto* ast = nodes[start++];
            for (auto* arg : ast->nodes) {
                argNames.push_back(arg->nodes[0]->token.str);
                argTypes.push_back(arg->nodes[1]->token.str);
            }
        }
        std::string ret = nodes[start++]->token.str;
        auto jmp = vm.WriteCommandDefFunc(name, ret, argTypes);

        auto setMemsize = vm.WriteAlloc();
        code.ClearTmp();
        pc = 1;

        int id = pc++;
        code.currentScope->varID["this"] = id;
        vm.WriteCommandArg(id, 0);

        for (int i = 0; i < argNames.size(); i++) {
            id = pc++;
            code.currentScope->varID[argNames[i]] = id;
            code.currentScope->varType[argNames[i]] = argTypes[i];
            vm.WriteCommandArg(id, i + 1);
        }

        SUB(start++);
        vm.WriteCommandRet(0);
        jmp(vm.GetProgress());
        setMemsize(pc);
        pc = 1;
        code.MoveBack();
    } else if (branch & 4) {
        // Field
        std::string varName = nodes[start++]->token.str;
        std::string typeName = nodes[start++]->token.str;
        compilingClassVars.push_back(varName);
        compilingClassVarTypes.push_back(typeName);
        vm.WriteCommandDefVar(typeName, varName);
        pc--;
        code.currentScope->varID.erase(varName);
        code.currentScope->varType.erase(typeName);
    } else if (branch & 16) {
        // using
        std::string name = nodes[start++]->token.str;
        compilingClassVars.push_back(name);
        compilingClassVarTypes.push_back(name);
        vm.WriteCommandNamespace(name);
        pc--;
        code.currentScope->varID.erase(name);
        code.currentScope->varType.erase(name);
    }
    return 0;
}

Compile(IDList) {
    START();
    return 0;
}

Compile(SingleAttr) {
    START();
    std::string attr = nodes[0]->token.str;
    std::vector<std::string> attrs;
    if (nodes.size() > 1) {
        for (int i = 0; i < nodes[1]->nodes.size(); i++) {
            attrs.push_back(nodes[1]->nodes[i]->token.str);
        }
    }
    vm.WriteCommandAttributes(attr, attrs);
    return 0;
}

Compile(Attribute) {
    START();
    for (int i = 0; i < nodes.size(); i++) {
        SUB(i);
    }
    return 0;
}

Compile(Preprocess) {
    START();
    // class def
    int start = 0;
    if (branch & 2) {
        SUB(start++);
    }
    std::string className = nodes[start++]->token.str;
    compilingClassName = className;
    compilingClassVars.clear();
    compilingClassVarTypes.clear();
    compilingClassVarID.clear();
    std::vector<std::string> inherits;
    if (branch & 4) {
        GosAST* idList = nodes[start++];
        for (GosAST* ast : idList->nodes) {
            inherits.push_back(ast->token.str);
        }
    }
    vm.WriteCommandAttributes("GosInstance", {});
    vm.WriteCommandDefClass(className, inherits);
    auto& mgr = ReflMgr::Instance();
    for (const std::string& cls : inherits) {
        mgr.IterateField(TypeID::getRaw(cls), std::function([&](const FieldInfo& field) {
            std::string varName = field.name;
            std::string typeName = (std::string)field.varType.getName();
            compilingClassVars.push_back(varName);
            compilingClassVarTypes.push_back(typeName);
            vm.WriteCommandDefVar(typeName, varName);
            pc--;
            code.currentScope->varID.erase(varName);
            code.currentScope->varType.erase(typeName);
        }));
    }
    for (int i = start; i < nodes.size(); i++) {
        SUB(i);
    }
    return 0;
}
