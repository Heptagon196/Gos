#include "Gos.h"
#include "Reflection/ReflMgrInit.h"
#include <iostream>
#include <fstream>

struct IO {
    static int i;
    int count() {
        return i--;
    }
    void print(float i) {
        std::cout << i << std::endl;
    }
    void print(bool i) {
        std::cout << (i ? "true" : "false") << std::endl;
    }
    void print(std::string i) {
        std::cout << i << std::endl;
    }
    void print(SharedObject i) {
        std::cout << i << std::endl;
    }
    void read(int& i) {
        std::cin >> i;
    }
    void read(std::string& i) {
        std::cin >> i;
    }
    void read(SharedObject& i) {
        i = SharedObject::New<int>();
        std::cin >> i.As<int>();
    }
    void Greatings() {
        std::cout << "hello world" << std::endl;
    }
    static void Hello() {
        std::cout << "hello world" << std::endl;
    }
};

struct Array {
    std::vector<SharedObject> cont;
    SharedObject& operator[] (int idx) {
        return cont[idx];
    }
};

struct unknown {};

int IO::i = 10;

void reflIO() {
    ReflMgrTool::Init();
    ReflMgrTool::AutoRegister<Array>();
    auto& refl = ReflMgr::Instance();
    refl.AddClass<IO>();
    refl.AddClass<unknown>();
    refl.AddMethod<Array>(std::function([](Array* self, int size) { self->cont.resize(size); }), "__ctor");
    refl.AddAliasClass("array", "Array");
    refl.AddAliasClass("string", TypeID::get<std::string>().getName());
    refl.AddMethod(std::function([](std::vector<SharedObject>* self, int size) { self->resize(size); }), "resize");
    refl.AddMethod(&IO::count, "count");
    refl.AddMethod(MethodType<void, IO, float>::Type(&IO::print), "print");
    refl.AddMethod(MethodType<void, IO, bool>::Type(&IO::print), "print");
    refl.AddMethod(MethodType<void, IO, std::string>::Type(&IO::print), "print");
    refl.AddMethod(MethodType<void, IO, SharedObject>::Type(&IO::print), "print");
    refl.AddMethod(MethodType<void, IO, int&>::Type(&IO::read), "read");
    refl.AddMethod(MethodType<void, IO, std::string&>::Type(&IO::read), "read");
    refl.AddMethod(MethodType<void, IO, SharedObject&>::Type(&IO::read), "read");
    refl.AddMethod(&IO::Greatings, "Greatings");
    refl.AddStaticMethod(TypeID::get<IO>(), &IO::Hello, "hello");
}

int vmTest(int argc, char* argv[]) {
    reflIO();
    GosVM::RTConst constArea;
    GosVM::VMProgram program(&constArea);
    std::ifstream fin;
    if (argc > 1) {
        fin.open(argv[1]);
    } else {
        fin.open("1.in");
    }
    program.Read(fin, true);
    fin.close();
    // program.Write(std::cout, true);
    auto ret = program.Execute();
    /*
    auto v = refl.New("Virtual");
    v.Invoke("print", {});
    v.GetField("x").As<int>() = 2333;
    v.Invoke("print", {});
    std::cout << v.GetField("x") << std::endl;
    */
    return 0;
}

int main(int argc, char* argv[]) {
    reflIO();
    Gos::GosProject prj;
    // prj.AddScript(name);
    prj.ScanDirectory();
    ReflMgr::Instance().New("Lambda").Invoke("main")({ SharedObject::New<int>(2333) });
    /*
    const char* name;
    if (argc > 1) {
        name = argv[1];
    } else {
        name = "Lambda.gos";
    }
    std::ifstream fin;
    fin.open(name);
    Gos::GosScript script;
    script.Read(fin, name);
    fin.close();
    script.PrintIR(std::cout);
    script.Execute();
    auto f = script.CreateInstance().Invoke("main", {});
    f({ SharedObject::New<int>(1) });
    f({ SharedObject::New<int>(2) });
    f({ SharedObject::New<int>(3) });
    ReflMgr::Instance().InvokeStatic(TypeID::getRaw("Lambda"), "Greatings", {});
    */
    return 0;
}
