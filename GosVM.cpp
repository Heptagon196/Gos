#include "GosVM.h"
#include <algorithm>
#include <stack>
#include <sstream>

namespace GosVM {
    struct OperatorInfo {
        std::string name;
    };
    const inline int operatorsCount = 34;
    static inline TagList currentTag;
    const inline OperatorInfo operatorInfos[] = {
        {"EOF"}, // NONE,

        {"class"}, // DEF_CLASS,
        {"attr"}, // ATTR
        {"decl_var"}, // DEF_MEMBER_VAR,
        {"decl_func"}, // DEF_MEMBER_FUNC,
        {"static_var"}, // DEF_STATIC_VAR,
        {"static_func"}, // DEF_STATIC_FUNC,

        {"alloc"}, // ALLOC
        {"str"}, // STR,
        {"num"}, // NUM,
        {"namespace"}, // NS,
        {"new"}, // NEW,
        {"new_str"}, // NEW_STR,
        {"new_num"}, // NEW_NUM,
        {"mov"}, // MOV,
        {"ref"}, // REF,
        {"clone"}, // CLONE,
        {"arg"}, // ARG,
        {"get_field"}, // GET_FIELD,

        {"add"}, // ADD,
        {"sub"}, // SUB,
        {"mul"}, // MUL,
        {"div"}, // DIV,
        {"rem"}, // REM,

        {"and"}, // AND,
        {"or"}, // OR,
        {"xor"}, // XOR,
        {"not"}, // NOT,

        {"ret"}, // RET,
        {"jmp"}, // JMP,
        {"if"}, // IF,
                //
        {"box"}, // BOX,
        {"unbox"}, // UNBOX,

        {"call"}, // CALL,
    };
}

size_t std::hash<GosVM::RTConstNum>::operator() (const GosVM::RTConstNum& num) const {
    return num.data.i64;
}

bool GosVM::RTConstNum::operator == (const RTConstNum& other) const {
    return data.i64 == other.data.i64;
}

int GosVM::RTConst::getStrID(const std::string& s) {
    if (strMap.find(s) == strMap.end()) {
        strs.push_back(std::make_shared<std::string>(s));
        strMap[s] = strs.size() - 1;
        return strs.size() - 1;
    }
    return strMap[s];
}

void GosVM::RTConst::setStrID(const std::string& s, int id) {
    auto cont = std::make_shared<std::string>(s);
    if (id < strs.size()) {
        strs[id] = cont;
    } else {
        while (id > strs.size()) {
            strs.push_back(nullptr);
        }
        strs.push_back(cont);
    }
    strMap[s] = id;
}

int GosVM::RTConst::getNumID(RTConstNum s) {
    if (numMap.find(s) == numMap.end()) {
        nums.push_back(s);
        numMap[s] = nums.size() - 1;
        return nums.size() - 1;
    }
    return numMap[s];
}

void GosVM::RTConst::setNumID(RTConstNum s, int id) {
    if (id < nums.size()) {
        nums[id] = s;
    } else {
        while (id > nums.size()) {
            nums.push_back({});
        }
        nums.push_back(s);
    }
    numMap[s] = id;
}

void GosVM::VMExecutable::StartRead() {
    readProgress = startPos;
}

GosVM::IRTokenizer::IRTokenizer(std::istream& input): input(input) {}

std::string GosVM::IRTokenizer::GetToken() {
    std::string ret = "";
    char ch;
    bool in_string = false;
    bool is_string = false;
    while ((ch = input.get()) != EOF) {
        if (ch == '\"') {
            if (!in_string) {
                in_string = true;
                is_string = true;
            } else {
                ret += "\"";
                break;
            }
        }
        if (ch == ' ' || ch == '\n' || ch == '\r') {
            if (!in_string) {
                break;
            }
        }
        ret += ch;
    }
    last = ch;
    if (is_string) {
        return ret.substr(1, ret.size() - 2);
    }
    return ret;
}

bool GosVM::IRTokenizer::IsEOL() const {
    return last == '\n' || last == '\r';
}

bool GosVM::IRTokenizer::IsEOF() const {
    return input.eof();
}

template<typename T> T GosVM::IRTokenizer::TokenTo(std::string token) {
    std::stringstream ss;
    ss << token;
    T i;
    ss >> i;
    return i;
}

GosVM::VMProgram::VMProgram(GosVM::RTConst* constArea): content(""), cst(constArea) {
    mem.mem.push_back(SharedObject::Null);
    startPos = 0;
}

void GosVM::VMProgram::AddOperation(char param) {
    content += param;
}

void GosVM::VMProgram::AddParamBool(char param) {
    content += param;
}

void GosVM::VMProgram::AddParamByte(char param) {
    content += param;
}

void GosVM::VMProgram::AddParamInt(int param) {
    for (int i = 0; i < 32; i += 8) {
        content += (char)((param >> i) & ((1 << 9) - 1));
    }
}

void GosVM::VMProgram::AddParamInt64(int64_t param) {
    for (int i = 0; i < 64; i += 8) {
        content += (char)((param >> i) & ((1 << 9) - 1));
    }
}

void GosVM::VMProgram::AddParamAddr(int64_t param) {
    AddParamInt64(param);
}

void GosVM::VMProgram::AddParamFloat(float param) {
    AddParamInt(*(int*)&param);
}

void GosVM::VMProgram::AddParamDouble(double param) {
    AddParamInt64(*(int64_t*)&param);
}

void GosVM::VMProgram::AddParamString(std::string param) {
    AddParamInt(cst->getStrID(param));
}

void GosVM::VMProgram::AddParamClass(int64_t param) {}

char& GosVM::VMExecutable::GetOperation() {
    readProgress += 1;
    return (*curContent)[readProgress - 1];
}

char& GosVM::VMExecutable::GetParamBool() {
    readProgress += 1;
    return (*curContent)[readProgress - 1];
}

char& GosVM::VMExecutable::GetParamByte() {
    readProgress += 1;
    return (*curContent)[readProgress - 1];
}

int& GosVM::VMExecutable::GetParamInt() {
    readProgress += 4;
    return *(int*)&(*curContent)[readProgress - 4];
}

int64_t& GosVM::VMExecutable::GetParamInt64() {
    readProgress += 8;
    return *(int64_t*)&(*curContent)[readProgress - 8];
}

int64_t& GosVM::VMExecutable::GetParamAddr() {
    readProgress += 8;
    return *(int64_t*)&(*curContent)[readProgress - 8];
}

float& GosVM::VMExecutable::GetParamFloat() {
    readProgress += 4;
    return *(float*)&(*curContent)[readProgress - 4];
}

double& GosVM::VMExecutable::GetParamDouble() {
    readProgress += 8;
    return *(double*)&(*curContent)[readProgress - 8];
}

std::string& GosVM::VMExecutable::GetParamString() {
    return *getConst().strs[GetParamInt()];
}

int& GosVM::VMExecutable::GetParamClass() {
    readProgress += 4;
    return *(int*)&(*curContent)[readProgress - 4];
}

GosVM::RTMemory& GosVM::VMFunction::getMem() {
    return *mem;
}

GosVM::RTConst& GosVM::VMFunction::getConst() {
    return program->getConst();
}

std::string& GosVM::VMFunction::getContent() {
    return program->getContent();
}

SharedObject GosVM::VMFunction::Execute(ObjectPtr instance, const std::vector<ObjectPtr>& params) {
    RTMemory funcMem;
    mem = &funcMem;
    return VMExecutable::Execute(instance, params);
}

GosVM::RTMemory& GosVM::VMProgram::getMem() {
    return mem;
}

GosVM::RTConst& GosVM::VMProgram::getConst() {
    return *cst;
}

std::string& GosVM::VMProgram::getContent() {
    return content;
}

GosVM::VMFunction::VMFunction() {}
GosVM::VMFunction::VMFunction(GosVM::VMExecutable* program, int progress) : program(program) {
    startPos = progress;
    endPos = INT_MAX;
}

GosVM::GosClass::GosClass() {}
GosVM::GosClass::GosClass(std::string className) : className(className) {}

void GosVM::VMProgram::Read(std::istream& input, bool prettified) {
    startPos = 0;
    if (prettified) {
        std::string command;
        IRTokenizer tokenizer(input);
        content = "";
        while (!tokenizer.IsEOF()) {
            command = tokenizer.GetToken();
            int op = 0;
            for (int i = 0; i < operatorsCount; i++) {
                if (operatorInfos[i].name == command) {
                    op = i;
                    break;
                }
            }
            AddOperation(op);
            if (op == DEF_CLASS) {
                AddParamString(tokenizer.GetToken());
                int len = tokenizer.GetToken<int>();
                AddParamInt(len);
                while (len--) {
                    AddParamString(tokenizer.GetToken());
                }
            } else if (op == ATTR) {
                AddParamString(tokenizer.GetToken());
                int len = tokenizer.GetToken<int>();
                AddParamInt(len);
                while (len--) {
                    AddParamString(tokenizer.GetToken());
                }
            } else if (op == DEF_MEMBER_VAR) {
                AddParamString(tokenizer.GetToken());
                AddParamString(tokenizer.GetToken());
            } else if (op == DEF_MEMBER_FUNC) {
                AddParamString(tokenizer.GetToken());
                AddParamString(tokenizer.GetToken());
                int len = tokenizer.GetToken<int>();
                AddParamInt(len);
                while (len--) {
                    AddParamString(tokenizer.GetToken());
                }
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == DEF_STATIC_VAR) {
                AddParamString(tokenizer.GetToken());
                AddParamString(tokenizer.GetToken());
            } else if (op == DEF_STATIC_FUNC) {
                AddParamString(tokenizer.GetToken());
                AddParamString(tokenizer.GetToken());
                int len = tokenizer.GetToken<int>();
                AddParamInt(len);
                while (len--) {
                    AddParamString(tokenizer.GetToken());
                }
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == ALLOC) {
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == STR) {
                int id = tokenizer.GetToken<int>();
                [[maybe_unused]] int len = tokenizer.GetToken<int>();
                std::string s = tokenizer.GetToken();
                cst->setStrID(s, id);
            } else if (op == NUM) {
                int id = tokenizer.GetToken<int>();
                RTConstNum num;
                num.data.i64 = tokenizer.GetToken<int64_t>();
                cst->setNumID(num, id);
            } else if (op == NS) {
                AddParamString(tokenizer.GetToken());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == NEW) {
                AddParamString(tokenizer.GetToken());
                AddParamInt(tokenizer.GetToken<int>());
                int len = tokenizer.GetToken<int>();
                AddParamInt(len);
                while (len--) {
                    AddParamInt(tokenizer.GetToken<int>());
                }
            } else if (op == NEW_STR) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamString(tokenizer.GetToken());
            } else if (op == NEW_NUM) {
                int8_t type = tokenizer.GetToken<int>();
                AddParamByte(type);
                AddParamInt(tokenizer.GetToken<int>());
                RTConstNum num;
                num.data.i64 = 0;
                std::stringstream ss;
                ss << tokenizer.GetToken();
                switch (type) {
                    case 0: ss >> num.data.i32; break;
                    case 1: ss >> num.data.i32; break;
                    case 2: ss >> num.data.i64; break;
                    case 3: ss >> num.data.f; break;
                    case 4: ss >> num.data.d; break;
                }
                AddParamInt(cst->getNumID(num));
            } else if (op == MOV) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == REF) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == CLONE) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == ARG) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == GET_FIELD) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
                AddParamString(tokenizer.GetToken());
            } else if (op == ADD) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == SUB) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == MUL) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == DIV) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == REM) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == AND) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == OR) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == XOR) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == NOT) {
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == RET) {
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == JMP) {
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == IF) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == BOX) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == UNBOX) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamInt(tokenizer.GetToken<int>());
            } else if (op == CALL) {
                AddParamInt(tokenizer.GetToken<int>());
                AddParamString(tokenizer.GetToken());
                int cnt = tokenizer.GetToken<int>();
                AddParamInt(cnt);
                for (int i = 0; i < cnt; i++) {
                    AddParamInt(tokenizer.GetToken<int>());
                }
            }
        }
    } else {
        std::stringstream buffer;
        buffer << input.rdbuf();
        content = buffer.str();
    }
    endPos = content.size() - 1;
}

void GosVM::VMExecutable::Write(std::ostream& out, bool prettified) {
    curContent = &getContent();
    auto& cst = getConst();
    if (prettified) {
        StartRead();
        while (readProgress < curContent->length() && readProgress < endPos) {
            out << readProgress << ' ';
            int op = GetOperation();
            out << operatorInfos[op].name << ' ';
            if (op == DEF_CLASS) {
                out << GetParamString() << ' ';
                int len = GetParamInt();
                out << len << ' ';
                while (len--) {
                    out << GetParamString() << ' ';
                }
                out << std::endl;
            } else if (op == ATTR) {
                out << GetParamString() << ' ';
                int len = GetParamInt();
                out << len << ' ';
                while (len--) {
                    out << GetParamString() << ' ';
                }
                out << std::endl;
            } else if (op == DEF_MEMBER_VAR) {
                out << GetParamString() << ' ';
                out << GetParamString() << std::endl;
            } else if (op == DEF_MEMBER_FUNC) {
                out << GetParamString() << ' ';
                out << GetParamString() << ' ';
                int len = GetParamInt();
                out << len << ' ';
                while (len--) {
                    out << GetParamString() << ' ';
                }
                out << GetParamInt() << std::endl;
            } else if (op == DEF_STATIC_VAR) {
                out << GetParamString() << ' ';
                out << GetParamString() << std::endl;
            } else if (op == DEF_STATIC_FUNC) {
                out << GetParamString() << std::endl;
                out << GetParamString() << ' ';
                int len = GetParamInt();
                out << len << ' ';
                while (len--) {
                    out << GetParamString() << ' ';
                }
                out << GetParamInt() << std::endl;
            } else if (op == ALLOC) {
                out << GetParamInt() << std::endl;
            } else if (op == NS) {
                out << GetParamString() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == NEW) {
                out << GetParamString() << ' ';
                out << GetParamInt() << ' ';
                int len = GetParamInt();
                out << len << ' ';
                while (len--) {
                    out << GetParamInt() << ' ';
                }
                out << std::endl;
            } else if (op == NEW_STR) {
                out << GetParamInt() << ' ';
                out << "\"" << GetParamString() << "\"" << std::endl;
            } else if (op == NEW_NUM) {
                int8_t type = GetParamByte();
                out << (int)type << ' ';
                out << GetParamInt() << ' ';
                RTConstNum num = cst.nums[GetParamInt()];
                switch (type) {
                    case 0: out << num.data.i8; break;
                    case 1: out << num.data.i32; break;
                    case 2: out << num.data.i64; break;
                    case 3: out << num.data.f; break;
                    case 4: out << num.data.d; break;
                }
                out << std::endl;
            } else if (op == MOV) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == REF) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == CLONE) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == ARG) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == GET_FIELD) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << ' ';
                out << GetParamString() << std::endl;
            } else if (op == ADD) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == SUB) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == MUL) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == DIV) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == REM) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == AND) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == OR) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == XOR) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == NOT) {
                out << GetParamInt() << std::endl;
            } else if (op == RET) {
                out << GetParamInt() << std::endl;
            } else if (op == JMP) {
                out << GetParamInt() << std::endl;
            } else if (op == IF) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == BOX) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == UNBOX) {
                out << GetParamInt() << ' ';
                out << GetParamInt() << std::endl;
            } else if (op == CALL) {
                out << GetParamInt() << ' ';
                out << GetParamString() << ' ';
                int cnt = GetParamInt();
                out << cnt << ' ';
                while (cnt--) {
                    out << GetParamInt() << ' ';
                }
                out << std::endl;
            }
        }
    } else {
        for (auto [s, id] : cst.strMap) {
            out << (char)STR;
            for (int i = 0; i < 32; i += 8) {
                out << (char)((id >> i) & ((1 << 9) - 1));
            }
            int len = s.length();
            for (int i = 0; i < 32; i += 8) {
                out << (char)((len >> i) & ((1 << 9) - 1));
            }
            out << s;
        }
        for (auto [s, id] : cst.numMap) {
            out << (char)NUM;
            for (int i = 0; i < 32; i += 8) {
                out << (char)((id >> i) & ((1 << 9) - 1));
            }
            int64_t val = s.data.i64;
            for (int i = 0; i < 64; i += 8) {
                out << (char)((val >> i) & ((1 << 9) - 1));
            }
        }
        out << (*curContent);
    }
}

std::unordered_map<TypeID, GosVM::GosClass> GosVM::GosClass::classInfo;
std::unordered_map<TypeID, std::unordered_map<std::string, SharedObject>> GosVM::GosClass::staticVars;

void GosVM::RTMemory::NewVar(int id, SharedObject obj) {
    if (id < mem.size()) {
        mem[id] = obj;
    } else {
        while (mem.size() < id) {
            mem.push_back(SharedObject::Null);
        }
        mem.push_back(obj);
    }
}

SharedObject GosVM::VMExecutable::Execute(ObjectPtr instance, const std::vector<ObjectPtr>& params) {
    int progressBackup = readProgress;
    auto& memMgr = getMem();
    auto& mem = memMgr.mem;
    auto& cst = getConst();
    curContent = &getContent();
    auto& refl = ReflMgr::Instance();
    std::string clsName;
    TypeID cls = Namespace::Global.Type();
    GosClass* clsInfo = nullptr;
    StartRead();
    while (readProgress < curContent->length() && readProgress < endPos) {
        int op = GetOperation();
        if (op == ATTR) {
            std::string attr = GetParamString();
            int len = GetParamInt();
            std::vector<std::string> attrs;
            while (len--) {
                attrs.push_back(GetParamString());
            }
            currentTag[attr] = attrs;
        } else if (op == DEF_CLASS) {
            clsName = GetParamString();
            int len = GetParamInt();
            std::vector<std::string> inherits;
            while (len--) {
                inherits.push_back(GetParamString());
            }
            cls = ReflMgr::GetType(clsName);
            if (!refl.HasClassInfo(cls)) {
                if (GosClass::classInfo.find(cls) == GosClass::classInfo.end()) {
                    GosClass::classInfo[cls] = GosClass(clsName);
                }
                refl.AddVirtualClass(clsName, [clsName](const std::vector<ObjectPtr>& params) {
                    auto ret = SharedObject::New<GosInstance>();
                    GosInstance& instance = ret.As<GosInstance>();
                    auto type = ReflMgr::GetType(clsName);
                    GosClass* clsInfo = &GosClass::classInfo[type];
                    instance.info = clsInfo;
                    ret.ForceTypeTo(type);
                    for (auto [type, name] : clsInfo->vars) {
                        instance.field[name] = ReflMgr::Instance().New(type);
                    }
                    ret.ctor(params);
                    return ret;
                }, currentTag);
                currentTag = {};
                for (std::string& s : inherits) {
                    refl.AddVirtualInheritance(clsName, s);
                }
            }
            clsInfo = &GosClass::classInfo[cls];
        } else if (op == DEF_MEMBER_VAR) {
            TypeID type = ReflMgr::GetType(GetParamString());
            std::string& name = GetParamString();
            clsInfo->vars.push_back({type, name});
            refl.RawAddField(cls, type, name, [name](ObjectPtr instance) {
                return instance.As<GosInstance>().field[name];
            });
            refl.GetFieldTag(cls, name) = currentTag;
            currentTag = {};
        } else if (op == DEF_MEMBER_FUNC) {
            std::string& name = GetParamString();
            std::string& retName = GetParamString();
            TypeID ret = ReflMgr::GetType(retName);
            ArgsTypeList typeList;
            int cnt = GetParamInt();
            for (int i = 0; i < cnt; i++) {
                typeList.push_back(ReflMgr::GetType(GetParamString()));
            }
            int jmp = GetParamInt();
            clsInfo->method[name] = VMFunction(this, readProgress);
            auto* func = &clsInfo->method[name];
            refl.RawAddMethod(cls, name, ret, typeList, [func](ObjectPtr instance, const std::vector<ObjectPtr>& params) {
                return func->Execute(instance, params);
            });
            refl.GetMethodInfo(cls, name, typeList) = currentTag;
            currentTag = {};
            readProgress += jmp;
        } else if (op == DEF_STATIC_VAR) {
            TypeID type = ReflMgr::GetType(GetParamString());
            std::string& name = GetParamString();
            GosClass::staticVars[cls][name] = refl.New(type);
            SharedObject* obj = &GosClass::staticVars[cls][name];
            refl.RawAddStaticField(cls, type, name, [obj]() {
                return *obj;
            });
            refl.GetFieldTag(cls, name) = currentTag;
            currentTag = {};
        } else if (op == DEF_STATIC_FUNC) {
            std::string& name = GetParamString();
            std::string& retName = GetParamString();
            TypeID ret = ReflMgr::GetType(retName);
            ArgsTypeList typeList;
            int cnt = GetParamInt();
            for (int i = 0; i < cnt; i++) {
                typeList.push_back(ReflMgr::GetType(GetParamString()));
            }
            clsInfo->method[name] = VMFunction(this, readProgress);
            auto* func = &clsInfo->method[name];
            refl.RawAddStaticMethod(cls, name, ret, typeList, [func](const std::vector<ObjectPtr>& params) {
                return func->Execute(ObjectPtr::Null, params);
            });
            refl.GetMethodInfo(cls, name, typeList) = currentTag;
            currentTag = {};
            int jmp = GetParamInt();
            readProgress += jmp;
        } else if (op == ALLOC) {
            int size = GetParamInt();
            mem.resize(size);
        } else if (op == NS) {
            auto& type = GetParamString();
            int id = GetParamInt();
            memMgr.NewVar(id, SharedObject{ refl.GetType(type), nullptr });
        } else if (op == NEW) {
            auto& type = GetParamString();
            int id = GetParamInt();
            int len = GetParamInt();
            std::vector<ObjectPtr> params;
            while (len--) {
                params.push_back(mem[GetParamInt()]);
            }
            memMgr.NewVar(id, refl.New(type, params));
        } else if (op == NEW_STR) {
            int id = GetParamInt();
            std::string& s = GetParamString();
            memMgr.NewVar(id, SharedObject::New<std::string>(s));
        } else if (op == NEW_NUM) {
            int8_t type = GetParamByte();
            int id = GetParamInt();
            RTConstNum num = cst.nums[GetParamInt()];
            SharedObject obj;
            switch (type) {
                case 0: obj = SharedObject::New<int8_t>(num.data.i8); break;
                case 1: obj = SharedObject::New<int32_t>(num.data.i32); break;
                case 2: obj = SharedObject::New<int64_t>(num.data.i64); break;
                case 3: obj = SharedObject::New<float>(num.data.f); break;
                case 4: obj = SharedObject::New<double>(num.data.d); break;
            }
            memMgr.NewVar(id, obj);
        } else if (op == MOV) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].assign(mem[b]);
        } else if (op == REF) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a] = mem[b];
        } else if (op == CLONE) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a] = refl.New(mem[b].GetType());
            mem[a].assign(mem[b]);
        } else if (op == ARG) {
            int a = GetParamInt();
            int b = GetParamInt();
            SharedObject obj;
            if (b == 0) {
                obj = instance;
            } else {
                obj = params[b - 1];
                if (obj.GetType().getHash() == TypeID::get<ReflMgr::Any>().getHash()) {
                    obj = obj.As<ReflMgr::Any>().ToSharedPtr();
                }
            }
            memMgr.NewVar(a, obj);
        } else if (op == GET_FIELD) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a] = mem[b].GetField(GetParamString());
        } else if (op == ADD) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].assign(mem[a] + mem[b]);
        } else if (op == SUB) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].assign(mem[a] - mem[b]);
        } else if (op == MUL) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].assign(mem[a] * mem[b]);
        } else if (op == DIV) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].assign(mem[a] / mem[b]);
        } else if (op == REM) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].assign(mem[a] % mem[b]);
        } else if (op == AND) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].As<bool>() = mem[a].Get<bool>() && mem[b].Get<bool>();
        } else if (op == OR) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].As<bool>() = mem[a].Get<bool>() || mem[b].Get<bool>();
        } else if (op == XOR) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a].assign(mem[a] ^ mem[b]);
        } else if (op == NOT) {
            int a = GetParamInt();
            mem[a].As<bool>() = !mem[a].Get<bool>();
        } else if (op == RET) {
            int a = GetParamInt();
            readProgress = progressBackup;
            return mem[a];
        } else if (op == JMP) {
            int a = GetParamInt();
            readProgress += a;
        } else if (op == IF) {
            int a = GetParamInt();
            int b = GetParamInt();
            if (mem[a].Get<bool>()) {
                readProgress += b;
            }
        } else if (op == BOX) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a] = SharedObject::New<SharedObject>(mem[b]);
        } else if (op == UNBOX) {
            int a = GetParamInt();
            int b = GetParamInt();
            mem[a] = mem[b].As<SharedObject>();
        } else if (op == CALL) {
            int i = GetParamInt();
            std::string& func = GetParamString();
            std::vector<ObjectPtr> params;
            int cnt = GetParamInt();
            while (cnt--) {
                int p = GetParamInt();
                params.push_back(mem[p]);
            }
            mem[0] = mem[i].Invoke(func, params);
            if (mem[0].GetType().getHash() == TypeID::get<ReflMgr::Any>().getHash()) {
                mem[0] = mem[0].As<ReflMgr::Any>().ToSharedPtr();
            }
        }
    }
    readProgress = progressBackup;
    return SharedObject::Null;
}

void GosVM::VMProgram::WriteCommandDefClass(const std::string& className, const std::vector<std::string>& inherits) {
    AddOperation(DEF_CLASS);
    AddParamString(className);
    AddParamInt(inherits.size());
    for (int i = 0; i < inherits.size(); i++) {
        AddParamString(inherits[i]);
    }
}

void GosVM::VMProgram::WriteCommandDefVar(const std::string& type, const std::string& varName) {
    AddOperation(DEF_MEMBER_VAR);
    AddParamString(type);
    AddParamString(varName);
}

std::function<void(int)> GosVM::VMProgram::WriteCommandDefFunc(const std::string& name, const std::string& retType, const std::vector<std::string>& paramTypes) {
    AddOperation(DEF_MEMBER_FUNC);
    AddParamString(name);
    AddParamString(retType);
    int len = paramTypes.size();
    AddParamInt(len);
    for (int i = 0; i < len; i++) {
        AddParamString(paramTypes[i]);
    }
    int pos = content.size();
    AddParamInt(0);
    int startJump = content.size();
    return [this, pos, startJump](int jmp) {
        *(int*)&content[pos] = jmp - startJump;
    };
}

void GosVM::VMProgram::WriteCommandDefStaticVar(const std::string& type, const std::string& varName) {
    AddOperation(DEF_STATIC_VAR);
    AddParamString(type);
    AddParamString(varName);
}

std::function<void(int)> GosVM::VMProgram::WriteCommandDefStaticFunc(const std::string& name, const std::string& retType, const std::vector<std::string>& paramTypes) {
    AddOperation(DEF_STATIC_FUNC);
    AddParamString(name);
    AddParamString(retType);
    int len = paramTypes.size();
    AddParamInt(len);
    for (int i = 0; i < len; i++) {
        AddParamString(paramTypes[i]);
    }
    int pos = content.size();
    AddParamInt(0);
    int startJump = content.size();
    return [this, pos, startJump](int jmp) {
        *(int*)&content[pos] = jmp - startJump;
    };
}

void GosVM::VMProgram::WriteCommandNamespace(const std::string& type, int varID) {
    AddOperation(NS);
    AddParamString(type);
    AddParamInt(varID);
}

void GosVM::VMProgram::WriteCommandNew(const std::string& type, int varID, const std::vector<int>& params) {
    AddOperation(NEW);
    AddParamString(type);
    AddParamInt(varID);
    AddParamInt(params.size());
    for (int i = 0; i < params.size(); i++) {
        AddParamInt(params[i]);
    }
}

void GosVM::VMProgram::WriteCommandNewStr(int varID, const std::string& val) {
    AddOperation(NEW_STR);
    AddParamInt(varID);
    AddParamString(val);
}

void GosVM::VMProgram::WriteCommandNewNum(int8_t type, int varID, RTConstNum num) {
    AddOperation(NEW_NUM);
    AddParamByte(type);
    AddParamInt(varID);
    // std::cout << (int)num.data.i64 << " ID: " << cst->getNumID(num) << std::endl;
    AddParamInt(cst->getNumID(num));
}

void GosVM::VMProgram::WriteCommandMov(int varA, int varB) {
    AddOperation(MOV);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandRef(int varA, int varB) {
    AddOperation(REF);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandClone(int varA, int varB) {
    AddOperation(CLONE);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandArg(int varID, int paramID) {
    AddOperation(ARG);
    AddParamInt(varID);
    AddParamInt(paramID);
}

void GosVM::VMProgram::WriteCommandGetField(int varID, int sourceVarID, const std::string& fieldName) {
    AddOperation(GET_FIELD);
    AddParamInt(varID);
    AddParamInt(sourceVarID);
    AddParamString(fieldName);
}

void GosVM::VMProgram::WriteCommandAdd(int varA, int varB) {
    AddOperation(ADD);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandSub(int varA, int varB) {
    AddOperation(SUB);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandMul(int varA, int varB) {
    AddOperation(MUL);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandDiv(int varA, int varB) {
    AddOperation(DIV);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandRem(int varA, int varB) {
    AddOperation(REM);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandXor(int varA, int varB) {
    AddOperation(XOR);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandAnd(int varA, int varB) {
    AddOperation(AND);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandOr(int varA, int varB) {
    AddOperation(OR);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandNot(int varA) {
    AddOperation(NOT);
    AddParamInt(varA);
}

void GosVM::VMProgram::WriteCommandRet(int varID) {
    AddOperation(RET);
    AddParamInt(varID);
}

std::function<void(int)> GosVM::VMProgram::WriteCommandJmp() {
    AddOperation(JMP);
    int pos = content.size();
    AddParamInt(0);
    int startJump = content.size();
    return [this, pos, startJump](int jmp) {
        *(int*)&content[pos] = jmp - startJump;
    };
}

std::function<void(int)> GosVM::VMProgram::WriteCommandIf(int varID) {
    AddOperation(IF);
    AddParamInt(varID);
    int pos = content.size();
    AddParamInt(0);
    int startJump = content.size();
    return [this, pos, startJump](int jmp) {
        *(int*)&content[pos] = jmp - startJump;
    };
}

void GosVM::VMProgram::WriteCommandBox(int varA, int varB) {
    AddOperation(BOX);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandUnbox(int varA, int varB) {
    AddOperation(UNBOX);
    AddParamInt(varA);
    AddParamInt(varB);
}

void GosVM::VMProgram::WriteCommandCall(int varID, const std::string& func, std::vector<int> paramsID) {
    AddOperation(CALL);
    AddParamInt(varID);
    AddParamString(func);
    int cnt = paramsID.size();
    AddParamInt(cnt);
    for (int i = 0; i < cnt; i++) {
        AddParamInt(paramsID[i]);
    }
}

void GosVM::VMProgram::WriteCommandAttributes(const std::string& attr, const std::vector<std::string>& params) {
    AddOperation(ATTR);
    AddParamString(attr);
    int len = params.size();
    AddParamInt(len);
    for (int i = 0; i < len; i++) {
        AddParamString(params[i]);
    }
}

void GosVM::VMProgram::WriteFinish() {
    endPos = content.length() - 1;
}

int GosVM::VMProgram::GetProgress() const {
    return content.size();
}

std::function<void(int)> GosVM::VMProgram::WriteAlloc() {
    AddOperation(ALLOC);
    int pos = content.size();
    AddParamInt(0);
    return [this, pos](int memSize) {
        *(int*)&content[pos] = memSize;
    };
}
