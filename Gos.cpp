#include "Gos.h"
using namespace std;

char buffer = 0;

bool skip_rest = false;
bool break_out = false;
bool exit_func = false;
Any exit_value = nullptr;

string this_refers_to = "";

static streambuf *InBuf = cin.rdbuf();
static streambuf *OutBuf = cout.rdbuf();
ifstream fin;
ofstream fout;

map<char, char> conv = {
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'"', '"'},
    {'\\', '\\'},
    {'\n', '\n'},
};

struct Reference {
    shared_ptr<any> val;
};

#define Operator(op)                                \
    [](vector<Any> args) -> Any {                   \
        if (args.size() == 0) {                     \
            return 0;                               \
        }                                           \
        Any ans = 0;                                \
        ans.Assign(args[0]);                        \
        for (int i = 1; i < args.size(); ++ i) {    \
            ans = ans op args[i];                   \
        }                                           \
        return ans;                                 \
    }                                               \

#define CompareOperator(op)                         \
    [](vector<Any> args) -> Any {                   \
        if (args.size() == 0) {                     \
            return 0;                               \
        }                                           \
        Any ans = 0;                                \
        ans.Assign(args[0]);                        \
        for (int i = 1; i < args.size(); ++ i) {    \
            ans = int(ans op args[i]);              \
        }                                           \
        return ans;                                 \
    }                                               \

#define BoolOperator(op)                            \
    [](vector<Any> args) -> Any {                   \
        if (args.size() == 0) {                     \
            return 0;                               \
        }                                           \
        Any ans = 0;                                \
        ans.Assign(args[0]);                        \
        for (int i = 1; i < args.size(); ++ i) {    \
            ans = int(ans.Int() op args[i].Int());  \
        }                                           \
        return ans;                                 \
    }                                               \

#define Func [](vector<Any> args) -> Any

map<string, function<Any(vector<Any>)>> funcs = {
    {"int", Func {
        if (args[0].GetType() == typeid(double)) {
            return int(args[0].Double());
        } else {
            stringstream ss;
            int out;
            ss << args[0];
            ss >> out;
            return out;
        }
    }},
    {"float", Func {
        if (args[0].GetType() == typeid(int)) {
            return double(args[0].Int());
        } else {
            stringstream ss;
            double out;
            ss << args[0];
            ss >> out;
            return out;
        }
    }},
    {"string", Func {
        stringstream ss;
        string out;
        ss << args[0];
        getline(ss, out);
        if (out[out.length() - 1] == '\n') {
            return out.substr(0, out.length() - 1);
        }
        return out;
    }},
    {"array", Func {
        return Any(0).Repeat(args);
    }},
    {"list", Func {
        vector<Any> ret(args.size());
        for (int i = 0; i < args.size(); i ++) {
            ret[i].Assign(args[i]);
        }
        return ret;
    }},
    {"struct", Func {
        map<string, Any> ret;
        for (int i = 0; i < args.size(); i ++) {
            ret[*args[i].cast<string>()] = 0;
        }
        return ret;
    }},
    {"sin", Func { return sin(args[0].Double()); }},
    {"cos", Func { return cos(args[0].Double()); }},
    {"tan", Func { return tan(args[0].Double()); }},
    {"asin", Func { return asin(args[0].Double()); }},
    {"acos", Func { return acos(args[0].Double()); }},
    {"atan", Func { return atan(args[0].Double()); }},
    {"exp", Func { return exp(args[0].Double()); }},
    {"sqrt", Func { return sqrt(args[0].Double()); }},
    {"pow", Func { return pow(args[0].Double(), args[1].Double()); }},
    {"ceil", Func { return (int) ceil(args[0].Double()); }},
    {"floor", Func { return (int) floor(args[0].Double()); }},
    {"abs", Func { return abs(args[0].Double()); }},
    {"max", Func { return args[0] > args[1] ? args[0] : args[1]; }},
    {"min", Func { return args[0] < args[1] ? args[0] : args[1]; }},
    {"log", Func {
        if (args.size() == 1) {
            return log(args[0].Double());
        } else if (args.size() == 2) {
            return log(args[0].Double()) / log(args[1].Double());
        }
        return 0;
    }},
    {"_", Func {
        if (args.size() == 0) {
            return 0;
        }
        return args[args.size() - 1];
    }},
    {"+", Operator(+)},
    {"-", Operator(-)},
    {"*", Operator(*)},
    {"/", Operator(/)},
    {"%", Func { return (int) args[0].Int() % args[1].Int(); }},
    {"==", CompareOperator(==)},
    {"!=", CompareOperator(!=)},
    {"<", CompareOperator(<)},
    {">", CompareOperator(>)},
    {"<=", CompareOperator(<=)},
    {">=", CompareOperator(>=)},
    {"&&", BoolOperator(&&)},
    {"||", BoolOperator(||)},
    {"|", BoolOperator(|)},
    {"&", BoolOperator(&)},
    {"^", BoolOperator(^)},
    {"!", Func { return args[0].Int() ? 0 : 1; }},
    {"~", Func { return ~args[0].Int(); }},
    {"=", Func {
        if (args[0].GetType() == typeid(Reference)) {
            Any tmp;
            tmp.LinkTo(args[0].cast<Reference>()->val);
            tmp.Assign(args[1]);
            return args[0];
        }
        args[0].Assign(args[1]);
        return args[0];
    }},
    {":=", Func {
        args[0].Assign(args[1]);
        return args[0];
    }},
    {"+=", Func {
        Any ans = args[0] + args[1];
        args[0].Assign(ans);
        return args[0];
    }},
    {"-=", Func {
        Any ans = args[0] - args[1];
        args[0].Assign(ans);
        return args[0];
    }},
    {"*=", Func {
        Any ans = args[0] * args[1];
        args[0].Assign(ans);
        return args[0];
    }},
    {"/=", Func {
        Any ans = args[0] / args[1];
        args[0].Assign(ans);
        return args[0];
    }},
    {"push", Func {
        for (int i = 1; i < args.size(); i ++) {
            args[0].PushBack(0);
            args[0][args[0].GetSize() - 1].Assign(args[i]);
        }
        return nullptr;
    }},
    {"pop", Func {
        args[0].PopBack();
        return nullptr;
    }},
    {"get", Func {
        return args[0][args[1].Int()];
    }},
    {"remove", Func {
        args[0].EraseAt(args[1].Int());
        return nullptr;
    }},
    {"size", Func {
        return args[0].GetSize();
    }},
    {"print", Func {
        for (const auto& x : args) {
            cout << x;
        }
        return nullptr;
    }},
    {"println", Func {
        for (const auto& x : args) {
            cout << x;
        }
        cout << endl;
        return nullptr;
    }},
    {"read", Func {
        int ret = 0;
        for (auto& x : args) {
            ret = (cin >> x) ? 1 : 0;
        }
        return ret;
    }},
    {"getline", Func {
        string s;
        int ret = getline(cin, s) ? 1 : 0;
        Any tmp = s;
        args[0].Assign(tmp);
        return ret;
    }},
    {"getchar", Func {
        string tmp;
        char ch;
        if (!cin.read(&ch, 1)) {
            return -1;
        }
        tmp.push_back(ch);
        return tmp;
    }},
    {"randint", Func {
        int L = args[0].Int();
        int R = args[1].Int();
        return rand() % (R - L) + L;
    }},
    {"return", Func {
        if (args.size() == 0) {
            return nullptr;
        }
        exit_func = true;
        exit_value.Assign(args[args.size() - 1]);
        return exit_value;
    }},
    {"continue", Func {
        skip_rest = true;
        return nullptr;
    }},
    {"break", Func {
        skip_rest = true;
        break_out = true;
        return nullptr;
    }},
    {"substr", Func {
        if (args.size() == 2) {
            return args[0].cast<string>()->substr(args[1].Int(), 1);
        }
        return args[0].cast<string>()->substr(args[1].Int(), args[2].Int());
    }},
    {"strlen", Func {
        return (int)args[0].cast<string>()->length();
    }},
    {"exit", Func {
        exit(args[0].Int());
    }},
    {"is_const", Func {
        return args[0].IsConst() ? 1 : 0;
    }},
    {"is_int", Func {
        return args[0].GetType() == typeid(int) ? 1 : 0;
    }},
    {"is_float", Func {
        return args[0].GetType() == typeid(double) ? 1 : 0;
    }},
    {"is_string", Func {
        return args[0].GetType() == typeid(string) ? 1 : 0;
    }},
    {"is_array", Func {
        return args[0].GetType() == typeid(vector<Any>) ? 1 : 0;
    }},
    {"time", Func {
        return gettime();
    }},
    {"system", Func {
        return (int)system(args[0].String().c_str());
    }},
    {"read_from", Func {
        if (args[0].String() == "/dev/stdin") {
            fin.close();
            cin.rdbuf(InBuf);
            return 1;
        }
        fin.open(args[0].String());
        if (!fin) {
            return 0;
        }
        cin.rdbuf(fin.rdbuf());
        return 1;
    }},
    {"print_to", Func {
        if (args[0].String() == "/dev/stdout") {
            fout.close();
            cout.rdbuf(OutBuf);
            return 1;
        }
        fout.open(args[0].String());
        if (!fout) {
            return 0;
        }
        cout.rdbuf(fout.rdbuf());
        return 1;
    }},
    {"append_to", Func {
        if (args[0].String() == "/dev/stdout") {
            fout.close();
            cout.rdbuf(OutBuf);
            return 1;
        }
        fout.open(args[0].String(), ios::app);
        if (!fout) {
            return 0;
        }
        cout.rdbuf(fout.rdbuf());
        return 1;
    }},
    {"ref", Func {
        return (Reference){args[0].getRef()};
    }},
    {"global", Func {
        args[0].Assign(args[1]);
        return nullptr;
    }},
};

map<string, bool> keywords = {
    {"for", true},
    {"if", true},
    {"while", true},
    {"func", true},
};

const int is_vector = 0;
const int is_string = 1;
const int is_int = 2;
const int is_double = 3;
const int is_var = 4;
const int is_func = 5;
const int is_op = 6;

bool is_constant(int x) {
    return x < 4;
}

#define is_left_brace(s) ((s) == "(" || (s) == "[" || (s) == "{")
#define is_right_brace(s) ((s) == ")" || (s) == "]" || (s) == "}")

stack<FILE*> fps;
vector<string> filenames;
stack<int> curfile;
stack<int> linecnt;
bool conv_to_string = false;

char GetChar(FILE *fp) {
    char ch = fgetc(fp);
    if (ch == '\n') {
        linecnt.top() ++;
    }
    return ch;
}

struct Element {
    int type;
    string content;
    int line;
    int file;
    Element(int type, string content, int line, int file) : type(type), content(content), line(line), file(file) {
    }
};

const string TypeName[] = {"vector", "string", "int", "double", "var", "func", "op"};
ostream& operator << (ostream& fout, const Element& x) {
    fout << "|" << TypeName[x.type] << "|" << x.content<< "|";
    return fout;
}

void Error(string msg, int line, int file) {
    __Error(filenames[file] + ":" + to_string(line) + ": " + msg);
    exit(1);
}

Element get_token() {
    static int last_token_type = -1;
    int ch, type;
    FILE *fp = fps.top();
    if (buffer == 0) {
        ch = GetChar(fp);
    } else {
        ch = buffer;
        buffer = 0;
        if (ch == ';') {
            return {is_op, ";", linecnt.top(), curfile.top()};
        }
        if (ch == '.') {
            conv_to_string = true;
            return {is_op, ".", linecnt.top(), curfile.top()};
        }
    }
    string ans = "";
    RECHECK:;
    while (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
        ch = GetChar(fp);
    }
    if (ch == '#') {
        string command = "";
        string argument = "";
        string *cur = &command;
        while (ch != '\n') {
            ch = GetChar(fp);
            if (ch == ' ') {
                cur = &argument;
                continue;
            }
            if (ch != '\n') {
                cur->push_back(ch);
            }
        }
        if (command.length() > 0 && command[0] == '#') {
            if (command == "#include") {
                fps.push(fopen(argument.c_str(), "r"));
                filenames.push_back(argument);
                curfile.push(filenames.size() - 1);
                linecnt.push(1);
                if (fps.top() == NULL) {
                    Error("Unknown file: " + argument, linecnt.top(), curfile.top());
                }
                return get_token();
            }
        }
        goto RECHECK;
    }
    ans = ch;
    if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}') {
        type = is_op;
        if (ch == '}') {
            buffer = ';';
            conv_to_string = false;
            return {last_token_type = is_op, "}", linecnt.top(), curfile.top()};
        } else {
            ch = GetChar(fp);
        }
    } else if (ch == '"') {
        type = is_string;
        ans.pop_back();
        while (ch != EOF) {
            ch = GetChar(fp);
            if (ch ==  '"') {
                break;
            }
            if (ch == '\\') {
                ch = conv[GetChar(fp)];
            }
            ans += ch;
        }
        ch = GetChar(fp);
    } else if (isdigit(ch) || ch == '.' || (ch == '-' && !is_constant(last_token_type))) {
        type = is_int;
        if (ch == '.') {
            type = is_double;
        }
        while (isdigit(ch = GetChar(fp)) || ch == '.' || ch == 'f') {
            if (ch == '.') {
                type = is_double;
            }
            if (ch == 'f') {
                type = is_double;
                ch = GetChar(fp);
                break;
            }
            ans += ch;
        }
        if (ans == "-") {
            type = is_op;
            if (ch == '=') {
                ans += ch;
                ch = GetChar(fp);
            }
        }
    } else if (isalpha(ch)) {
        type = is_func;
        while (isalpha(ch = GetChar(fp)) || isdigit(ch) || ch == '_') {
            ans += ch;
        }
        while (ch == ' ') {
            ch = GetChar(fp);
        }
        if (ch != '(' && ch != '[' && ch != '{') {
            type = is_var;
            if (ch == '.') {
                buffer = '.';
                conv_to_string = false;
                return {last_token_type = type, ans, linecnt.top(), curfile.top()};
            }
            if (conv_to_string) {
                conv_to_string = false;
                type = is_string;
            }
        } else {
            ch = GetChar(fp);
        }
    } else {
        type = is_op;
        while (((ch = GetChar(fp)) != EOF) && !isdigit(ch) && !isalpha(ch) && ch != '.' && ch !='_' && ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t' && ch != '(' && ch != '[' && ch != '{' && ch != '"') {
            ans += ch;
            if (ans.length() == 2) {
                if (ans[1] == '-' && ans[0] != '-') {
                    ans.pop_back();
                    ch = '-';
                    break;
                } else {
                    ch = GetChar(fp);
                    break;
                }
            }
        }
    }
    if (ch != EOF) {
        while (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
            ch = GetChar(fp);
        }
        if (ch != EOF) {
            buffer = ch;
        }
    }
    conv_to_string = false;
    last_token_type = type;
    return {type, ans, linecnt.top(), curfile.top()};
}

class AST {
    public:
        Element element;
        map<string, Any> var;
        vector<AST*> node;
        AST* fa = this;
        //AST* lastif;
        bool endpoint = false;

        AST(const Element& element) : element(element) {}
        AST* last() {
            return node[node.size() - 1];
        }
        void newNode(const Element& token) {
            node.push_back(new AST(token));
            last()->fa = this;
        }
        template<typename T> void addVar(string name, const T& val) {
            var[name] = val;
        }
        template<typename T> void addConst(string name, const T& val) {
            var[name] = val;
            var[name].SetConst();
        }
        void addVar(string name, Any& val) {
            var[name].Assign(val);
        }
        void addConst(string name, Any& val) {
            var[name].Assign(val);
            var[name].SetConst();
        }
        Any& getVar(string name) {
            AST* cur = this;
            if ((fa->element.type == is_func || fa->element.type == is_op) && this == fa->node[0]) {
                if (fa->element.content == ":=") {
                    if (fa->fa->var.find(name) == fa->fa->var.end()) {
                        fa->fa->var[name] = 0;
                    }
                    return fa->fa->var[name];
                } else if (fa->element.content == "global") {
                    while (cur != cur->fa) {
                        cur = cur->fa;
                    }
                    cur->var[name] = 0;
                    return cur->var[name];
                }
            }
            while (cur != cur->fa) {
                if (cur->var.find(name) != cur->var.end()) {
                    return cur->var[name];
                }
                cur = cur->fa;
            }
            if (cur->var.find(name) != cur->var.end()) {
                return cur->var[name];
            }
            Error("No such variable: " + name, element.line, element.file);
            return var[name];
        }
        Any getConst() {
            if (!is_constant(element.type)) {
                Error("Not a number.", element.line, element.file);
            }
            stringstream ss;
            ss << element.content;
            Any ret;
#define conv_to(x)                              \
            if (element.type == is_ ## x) {    \
                x tmp;                          \
                ss >> tmp;                      \
                ret = tmp;                      \
                ret.SetConst();                 \
                return ret;                     \
            }
            conv_to(int);
            conv_to(double);
            if (element.type == is_string) {
                return element.content;
            }
            return 0;
        }
        Any Run();
        function<Any(vector<Any>)> getFunc();
        void print(int dep) {
#define indent()                                \
            for (int i = 0; i < dep; ++ i) {    \
                cout << "    ";                 \
            }
            indent();
            cout << element << endl;
            for (auto& x : node) {
                x->print(dep + 1);
            }
        }
};

struct GosFunc {
    vector<Any> f;
    AST* rt;
};

Any AST::Run() {
    if (exit_func) {
        if (endpoint) {
            exit_func = false;
            if (!(element.type == is_op && element.content == "return")) {
                return exit_value;
            }
        } else {
            return nullptr;
        }
    }
    if (skip_rest) {
        return nullptr;
    }
    if (element.type == is_func || element.type == is_op) {
        auto f = getFunc();
        if (f == nullptr) {
            if (element.content == "for") {
                for (node[0]->Run(); node[1]->Run().Int(); node[2]->Run()) {
                    node[3]->Run();
                    skip_rest = false;
                    if (break_out || exit_func) {
                        break_out = false;
                        break;
                    }
                }
            } else if (element.content == "if") {
                if (node[0]->Run().Int()) {
                    return node[1]->Run();
                } else if (node.size() > 2) {
                    return node[2]->Run();
                }
            } else if (element.content == "while") {
                while (node[0]->Run().Int()) {
                    node[1]->Run();
                    skip_rest = false;
                    if (break_out || exit_func) {
                        break_out = false;
                        break;
                    }
                }
            } else if (element.content == "func") {
                GosFunc f;
                for (int i = 0; i < node.size() - 1; i ++) {
                    f.f.push_back(var[node[i]->element.content]);
                }
                f.rt = last();
                return f;
            }
            return nullptr;
        } else {
            vector<Any> args;
            if (element.content != ".") {
                for (auto& x : node) {
                    args.push_back(x->Run());
                }
            } else {
                for (auto& x : node[1]->node) {
                    args.push_back(x->Run());
                }
            }
            return f(args);
        }
    }
    if (is_constant(element.type)) {
        return getConst();
    } else {
        return getVar(element.content);
    }
}

function<Any(vector<Any>)> AST::getFunc() {
    if (element.type != is_func && element.type != is_op) {
        Error("Not a function.", element.line, element.file);
    }
    if (keywords[element.content]) {
        return nullptr;
    }
    if (element.content == ".") {
        Any& f = (*getVar(node[0]->element.content == "this" ? this_refers_to : node[0]->element.content).cast<map<string, Any>>())[node[1]->element.content];
        if (f.GetType() != typeid(GosFunc)) {
            if (f.GetType() == typeid(vector<Any>)) {
                return [&](vector<Any> args) -> Any {
                    return f[args];
                };
            } else {
                return [&](vector<Any> args) -> Any {
                    return f;
                };
            }
        } else {
            return [&](vector<Any> args) -> Any {
                string tmp = this_refers_to;
                if (node[0]->element.content != "this") {
                    this_refers_to = node[0]->element.content;
                }
                vector<Any>& vec = f.cast<GosFunc>()->f;
                AST *rt = f.cast<GosFunc>()->rt;
#define CallFunction()                                                  \
                int size = min(vec.size(), args.size());                \
                vector<Any> bakvec(size);                               \
                for (int i = 0; i < size; i ++) {                       \
                    bakvec[i].Assign(vec[i]);                           \
                    vec[i].Assign(args[i]);                             \
                }                                                       \
                Any ret = rt->Run();                                    \
                for (int i = 0; i < size; i ++) {                       \
                    vec[i].Assign(bakvec[i]);                           \
                }
                CallFunction();
                this_refers_to = tmp;
                return ret;
            };
        }
    }
    //cout << "Calling function " << element.content << endl;
    if (funcs.find(element.content) == funcs.end()) {
        Any &f = getVar(element.content == "this" ? this_refers_to : element.content);
        if (f.GetType() == typeid(map<string, Any>)) {
            return [&](vector<Any> args) -> Any {
                string tmp = this_refers_to;
                if (element.content != "this") {
                    this_refers_to = element.content;
                }
                Any &t = (*f.cast<map<string, Any>>())["at"];
                vector<Any>& vec = t.cast<GosFunc>()->f;
                AST *rt = t.cast<GosFunc>()->rt;
                CallFunction();
                this_refers_to = tmp;
                return ret;
            };
        } else if (f.GetType() == typeid(vector<Any>)) {
            return [&](vector<Any> args) -> Any {
                return f[args];
            };
        } else if (f.GetType() == typeid(GosFunc)) {
            return [&](vector<Any> args) -> Any {
                vector<Any>& vec = f.cast<GosFunc>()->f;
                AST *rt = f.cast<GosFunc>()->rt;
                CallFunction();
                return ret;
            };
        }
        Error("Unknown function: " + element.content, element.line, element.file);
    }
    return funcs[element.content];
}
#undef CallFunction

stack<AST*> lastif;
void Build(AST* rt, stack<Element>& st) {
    //while (!st.empty() && (st.top().type == is_op && (st.top().content == "(" || st.top().content == ")"))) {
        //st.pop();
    //}
    if (st.empty()) {
        return ;
    }
    auto p = st.top();
    if (p.type == is_var && (p.content == "else" || p.content == "break" || p.content == "continue")) {
        p.type = is_func;
    }
    //cout << p << " is built" << endl;
    st.pop();
    if (!rt->node.empty() && (rt->last()->element.type == is_func && rt->last()->element.content == "if")) {
        if (!(p.type == is_op && p.content == "else")) {
            //lastif.pop();
        }
    }
    rt->newNode(p);
    if (is_constant(p.type) || (p.type == is_var)) {
        return ;
    }
    if (p.type == is_func) {
        //if (p.content == "if") {
            //lastif.push(rt->last());
            ////rt->fa->lastif = rt->last();
        //}
        if (p.content == "else") {
            AST *curlastif = lastif.top();
            //AST *curlastif = rt->fa->lastif;
            rt->node.pop_back();
            curlastif->newNode({is_func, "_", curlastif->last()->element.line, curlastif->last()->element.file});
            //rt->fa->fa->fa->fa->fa->fa->fa->fa->print(0);
            //getchar();
            bool has_brace = false;
            if (is_left_brace(st.top().content)) {
                has_brace = true;
            }
            if (has_brace) {
                st.pop();
            }
            if (has_brace) {
                while (!st.empty() && !(st.top().type == is_op && is_right_brace(st.top().content))) {
                    Build(curlastif->last(), st);
                }
            } else {
                Build(curlastif->last(), st);
            }
            if (has_brace) {
                st.pop();
            }
        } else if (p.content == "break" || p.content == "continue") {
            //rt->fa->fa->fa->fa->fa->fa->fa->fa->print(0);
        } else {
            st.pop();
            while (!st.empty() && !(st.top().type == is_op && is_right_brace(st.top().content))) {
                Build(rt->last(), st);
            }
            st.pop();
        }
        if (keywords[p.content]) {
            Build(rt->last(), st);
        }
        if (p.content == "if") {
            if (!st.empty() && ((st.top().type == is_func || st.top().type == is_var) && st.top().content == "else")) {
                lastif.push(rt->last());
            }
        }
        if (p.content == "func") {
            rt->last()->last()->newNode({is_func, "_", rt->last()->last()->last()->element.line, rt->last()->last()->last()->element.file});
            rt->last()->last()->last()->endpoint = true;
        }
        return ;
    }
    if (p.type == is_op) {
        Build(rt->last(), st);
        if (p.content != "!" && p.content != "~") {
            Build(rt->last(), st);
        }
        return ;
    }
    if (st.top().type != is_op || is_left_brace(st.top().content)) {
        Error("Not a function.", st.top().line, st.top().file);
    }
    //st.pop();
    while (st.top().type != is_op && is_right_brace(st.top().content)) {
        Build(rt->last(), st);
    }
    //st.pop();
}

map<string, int> priority = {
    {"=", 0},
    {":=", 0},
    {"+=", 0},
    {"-=", 0},
    {"*=", 0},
    {"/=", 0},
    {"||", 1},
    {"&&", 2},
    {"|", 3},
    {"^", 4},
    {"&", 5},
    {"==", 6},
    {"!=", 6},
    {"<", 7},
    {">", 7},
    {"<=", 7},
    {">=", 7},
    {"+", 8},
    {"-", 8},
    {"*", 9},
    {"/", 9},
    {"%", 9},
    {"!", 10},
    {"~", 10},
    {".", 11},
};

AST* root = new AST({is_func, "_", 1, 0});

void Gos::ClearGos() {
    if (root != nullptr) {
        delete root;
    }
    root = new AST({is_func, "_", 1, 0});
}

void Gos::ImportDefaultLib() {
    funcs["exec"] = Func {
        if (args.size() > 1) {
            return Gos::ExecuteFunc(Gos::BuildGos(args[0].String().c_str()), {args[1]});
        } else {
            return Gos::ExecuteFunc(Gos::BuildGos(args[0].String().c_str()), {(vector<Any>){}});
        }
    };
    root->addConst("PI", 3.1415926535);
    root->addConst("E", 2.718281828459);
    root->addConst("true", 1);
    root->addConst("false", 0);
    root->addConst("NULL", 0);
    root->addConst("EOF", -1);
    root->addConst("BLACK", BLACK);
    root->addConst("BLUE", BLUE);
    root->addConst("GREEN",  GREEN);
    root->addConst("CYAN", CYAN);
    root->addConst("RED", RED);
    root->addConst("PURPLE", RED);
    root->addConst("YELLOW", YELLOW);
    root->addConst("WHITE", WHITE);
#if defined(linux) || defined(__APPLE__)
    root->addConst("LINUX", 1);
#else
    root->addConst("LINUX", 0);
#endif
    funcs["gotoxy"] = Func {
        gotoxy(args[0].Int(), args[1].Int());
        return nullptr;
    };
    funcs["color"] = Func {
        color(args[0].Int(), args[1].Int());
        return nullptr;
    };
    funcs["getch"] = Func {
        string s = "";
        s.push_back(getch());
        return s;
    };
    funcs["kbhit"] = Func {
        return kbhit() ? 1 : 0;
    };
    funcs["hidecursor"] = Func {
        hidecursor();
        return nullptr;
    };
    funcs["showcursor"] = Func {
        showcursor();
        return nullptr;
    };
    funcs["clearcolor"] = Func {
        clearcolor();
        return nullptr;
    };
    funcs["clearscreen"] = Func {
        clearscreen();
        return nullptr;
    };
    funcs["readkey"] = Func {
        string s = "";
        s.push_back(readkey(args[0].Double()));
        return s;
    };
    // Debug function
    funcs["printAST"] = [&](vector<Any> args) -> Any {
        root->print(0);
        return nullptr;
    };
}

void Gos::ImportConst(map<string, Any> consts) {
    for (auto& x : consts) {
        root->addConst(x.first, x.second);
    }
}

void Gos::ImportVar(map<string, Any> vars) {
    for (auto& x : vars) {
        root->addConst(x.first, x.second);
    }
}

void Gos::ImportFunc(map<string, function<Any(vector<Any>)>> func) {
    for (auto& x : func) {
        funcs[x.first] = x.second;
    }
}

Any& Gos::GetVar(string name) {
    return root->getVar(name);
}

Any Gos::BuildGos(const char filename[]) {
    if (filename != nullptr && strlen(filename) != 0) {
        fps.push(fopen(filename, "r"));
        filenames.push_back(filename);
        curfile.push(filenames.size() - 1);
        linecnt.push(1);
        if (fps.top() == NULL) {
            fps.pop();
            Error("Unknown file: " + (string)filename, linecnt.top(), 0);
        }
    } else {
        fps.push(stdin);
        filenames.push_back("/dev/stdin");
        curfile.push(filenames.size() - 1);
        linecnt.push(1);
    }
    stack<Element> result, tmp, orig;
    while (true) {
        auto i = get_token();
        if (i.content.length() > 0 && i.content[0] == EOF) {
            fps.pop();
            linecnt.pop();
            curfile.pop();
            if (fps.empty()) {
                break;
            } else {
                continue;
            }
        }
        orig.push(i);
    }
    //tmp.push({is_func, "_"});
    while (!orig.empty()) {
        auto p = orig.top();
        orig.pop();
        if (p.type == is_op && (p.content == ";"  || p.content == ",")) {
            while (!tmp.empty() && !(tmp.top().type == is_op && is_right_brace(tmp.top().content))) {
                result.push(tmp.top());
                tmp.pop();
            }
            continue;
        }
        if (is_constant(p.type) || p.type == is_var) {
            result.push(p);
        } else if (p.type == is_op && is_right_brace(p.content)) {
            result.push(p);
            tmp.push(p);
        } else if ((p.type == is_op && is_left_brace(p.content)) || p.type == is_func) {
            while (!is_right_brace(tmp.top().content)) {
                result.push(tmp.top());
                tmp.pop();
            }
            result.push({is_op, "(", p.line, p.file});
            tmp.pop();
            if (p.type == is_func) {
                result.push(p);
            } else {
                result.push({is_func, "_", p.line, p.file});
            }
        } else {
            while (true) {
                if (tmp.empty() || (tmp.top().type == is_op && is_right_brace(tmp.top().content)) || priority[p.content] >= priority[tmp.top().content]) {
                    tmp.push(p);
                    break;
                } else {
                    result.push(tmp.top());
                    tmp.pop();
                }
            }
        }
    }
    while (!tmp.empty()) {
        result.push(tmp.top());
        tmp.pop();
    }
    //while (!result.empty()) {
        //cout << result.top() << endl;
        //result.pop();
    //}
    //return 0;
    root->newNode({is_func, "_", 1, 0});
    AST *cur = root->last();
    while (!result.empty()) {
        Build(cur, result);
    }
    cur->newNode({is_func, "_", cur->last()->element.line, cur->last()->element.file});
    cur->last()->endpoint = true;
    cur->addVar("args", 0);
    return (GosFunc){{cur->getVar("args")}, cur};
}

Any Gos::ExecuteFunc(Any func, vector<Any> args) {
    vector<Any>& vec = func.cast<GosFunc>()->f;
    int size = min(vec.size(), args.size());
    vector<Any> bakvec(size);
    for (int i = 0; i < size; i ++) {
        bakvec[i].Assign(vec[i]);
        vec[i].Assign(args[i]);
    }
    Any ret = func.cast<GosFunc>()->rt->Run();
    for (int i = 0; i < size; i ++) {
        vec[i].Assign(bakvec[i]);
    }
    return ret;
}

