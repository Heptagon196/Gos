#include "Gos.h"
using namespace std;

ostream& operator << (ostream& fout, const pair<int, string>& x) {
    fout << "|" << x.first << "|" << x.second << "|";
    return fout;
}

char buffer = 0;

bool skip_rest = false;
bool break_out = false;
bool exit_func = false;
Any exit_value = nullptr;

FILE *fp;

map<char, char> conv = {
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'"', '"'},
    {'\\', '\\'},
    {'\n', '\n'},
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
    {"int", Func { return int(args[0].Double()); }},
    {"float", Func { return double(args[0].Int()); }},
    {"string", Func {
        stringstream ss;
        string out;
        ss << args[0];
        ss >> out;
        return out;
    }},
    {"array", Func {
        return Any(0).Repeat(args);
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
            return nullptr;
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
            args[0].PushBack(args[1]);
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
    {"size", Func {
        return args[0].GetSize();
    }},
    //{"alias", Func {
        //args[0].LinkTo(args[1]);
        //return nullptr;
    //}},
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
        for (auto& x : args) {
            cin >> x;
        }
        return nullptr;
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
};

map<string, bool> keywords = {
    {"for", true},
    {"if", true},
    {"while", true},
    {"func", true},
};

bool end_of_file = false;

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

pair<int, string> get_token() {
    static int last_token_type = -1;
    int ch, type;
    if (buffer == 0) {
        ch = fgetc(fp);
    } else {
        ch = buffer;
        buffer = 0;
        if (ch == ';') {
            return {is_op, ";"};
        }
    }
    string ans = "";
    RECHECK:;
    while (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
        ch = fgetc(fp);
    }
    if (ch == '#') {
        while (ch != '\n') {
            ch = fgetc(fp);
        }
        goto RECHECK;
    }
    ans = ch;
    if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}') {
        type = is_op;
        if (ch == '}') {
            ch = ';';
            return {is_op, "}"};
        } else {
            ch = fgetc(fp);
        }
    } else if (ch == '"') {
        type = is_string;
        ans.pop_back();
        while (ch != EOF) {
            ch = fgetc(fp);
            if (ch ==  '"') {
                break;
            }
            if (ch == '\\') {
                ch = conv[fgetc(fp)];
            }
            ans += ch;
        }
        ch = fgetc(fp);
    } else if (isdigit(ch) || ch == '.' || (ch == '-' && !is_constant(last_token_type))) {
        type = is_int;
        if (ch == '.') {
            type = is_double;
        }
        while (isdigit(ch = fgetc(fp)) || ch == '.' || ch == 'f') {
            if (ch == '.') {
                type = is_double;
            }
            if (ch == 'f') {
                type = is_double;
                ch = fgetc(fp);
                break;
            }
            ans += ch;
        }
        if (ans == "-") {
            type = is_op;
            if (ch == '=') {
                ans += ch;
                ch = fgetc(fp);
            }
        }
    } else if (isalpha(ch)) {
        type = is_func;
        while (isalpha(ch = fgetc(fp)) || ch == '_') {
            ans += ch;
        }
        while(ch == ' ') {
            ch = fgetc(fp);
        }
        if (ch != '(' && ch != '[' && ch != '{') {
            type = is_var;
        } else {
            ch = fgetc(fp);
        }
    } else {
        type = is_op;
        while (((ch = fgetc(fp)) != EOF) && !isdigit(ch) && !isalpha(ch) && ch != '.' && ch !='_' && ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t' && ch != '(' && ch != '[' && ch != '{' && ch != '"') {
            ans += ch;
            if (ans.length() == 2) {
                if (ans[1] == '-' && ans[0] != '-') {
                    ans.pop_back();
                    ch = '-';
                    break;
                } else {
                    ch = fgetc(fp);
                    break;
                }
            }
        }
    }
    if (ch != EOF) {
        while (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
            ch = fgetc(fp);
        }
        if (ch == EOF) {
            end_of_file = true;
        } else {
            buffer = ch;
        }
    } else {
        end_of_file = true;
    }
    last_token_type = type;
    //cout << "Token: " << type << " |" << ans << "|" << endl;
    return {type, ans};
}

class AST {
    public:
        pair<int, string> element;
        map<string, Any> var;
        vector<AST*> node;
        AST* fa = this;
        //AST* lastif;
        bool endpoint = false;

        struct GosFunc {
            vector<Any> f;
            AST* rt;
        };

        AST() {}
        AST(const pair<int, string>& element) : element(element) {}
        AST* last() {
            return node[node.size() - 1];
        }
        void newNode(const pair<int, string>& token) {
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
            while (cur != cur->fa) {
                if (cur->var.find(name) != cur->var.end()) {
                    return cur->var[name];
                }
                cur = cur->fa;
            }
            if (cur->var.find(name) != cur->var.end()) {
                return cur->var[name];
            }
            if ((fa->element.first == is_func || fa->element.first == is_op) && fa->element.second == "=") {
                fa->fa->var[name] = 0;
                return fa->fa->var[name];
            }
            cur->var[name] = 0;
            return cur->var[name];
        }
        Any Run() {
            if (exit_func) {
                if (endpoint) {
                    exit_func = false;
                    if (!(element.first == is_op && element.second == "return")) {
                        return exit_value;
                    }
                } else {
                    return nullptr;
                }
            }
            if (skip_rest) {
                return nullptr;
            }
            if (element.first == is_func || element.first == is_op) {
                auto f = getFunc();
                if (f == nullptr) {
                    if (element.second == "for") {
                        for (node[0]->Run(); node[1]->Run().Int(); node[2]->Run()) {
                            node[3]->Run();
                            skip_rest = false;
                            if (break_out || exit_func) {
                                break_out = false;
                                break;
                            }
                        }
                    } else if (element.second == "if") {
                        if (node[0]->Run().Int()) {
                            return node[1]->Run();
                        } else if (node.size() > 2) {
                            return node[2]->Run();
                        }
                    } else if (element.second == "while") {
                        while (node[0]->Run().Int()) {
                            node[1]->Run();
                            skip_rest = false;
                            if (break_out || exit_func) {
                                break_out = false;
                                break;
                            }
                        }
                    } else if (element.second == "func") {
                        GosFunc f;
                        for (int i = 0; i < node.size() - 1; i ++) {
                            f.f.push_back(var[node[i]->element.second]);
                        }
                        f.rt = last();
                        return f;
                    }
                    return nullptr;
                } else {
                    vector<Any> args;
                    for (auto& x : node) {
                        args.push_back(x->Run());
                    }
                    return f(args);
                }
            }
            if (is_constant(element.first)) {
                return getConst();
            } else {
                return getVar(element.second);
            }
        }
        Any getConst() {
            if (!is_constant(element.first)) {
                Error("Not a number.");
            }
            stringstream ss;
            ss << element.second;
            Any ret;
#define conv_to(x)                              \
            if (element.first == is_ ## x) {    \
                x tmp;                          \
                ss >> tmp;                      \
                ret = tmp;                      \
                ret.SetConst();                 \
                return ret;                     \
            }
            conv_to(int);
            conv_to(double);
            if (element.first == is_string) {
                return element.second;
            }
            return 0;
        }
        function<Any(vector<Any>)> getFunc() {
            if (element.first != is_func && element.first != is_op) {
                Error("Not a function.");
            }
            if (keywords[element.second]) {
                return nullptr;
            }
            //cout << "Calling function " << element.second << endl;
            if (funcs.find(element.second) == funcs.end()) {
                Any &f = getVar(element.second);
                if (f.GetType() == typeid(vector<Any>)) {
                    return [&](vector<Any> args) -> Any {
                        return f[args];
                    };
                } else if (f.GetType() == typeid(GosFunc)) {
                    return [&](vector<Any> args) -> Any {
                        vector<Any>& vec = f.cast<GosFunc>()->f;
                        AST* rt = f.cast<GosFunc>()->rt;
                        vector<Any> bakvec(vec.size());
                        for (int i = 0; i < vec.size(); i ++) {
                            bakvec[i].Assign(vec[i]);
                            vec[i].Assign(args[i]);
                        }
                        Any ret = rt->Run();
                        for (int i = 0; i < vec.size(); i ++) {
                            vec[i].Assign(bakvec[i]);
                        }
                        return ret;
                    };
                }
                Error("Unknown function: " + element.second);
            }
            return funcs[element.second];
        }
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

stack<AST*> lastif;
void Build(AST* rt, stack<pair<int, string>>& st) {
    //while (!st.empty() && (st.top().first == is_op && (st.top().second == "(" || st.top().second == ")"))) {
        //st.pop();
    //}
    if (st.empty()) {
        return ;
    }
    auto p = st.top();
    if (p.first == is_var && (p.second == "else" || p.second == "break" || p.second == "continue")) {
        p.first = is_func;
    }
    //cout << p << " is built" << endl;
    st.pop();
    if (!rt->node.empty() && (rt->last()->element.first == is_func && rt->last()->element.second == "if")) {
        if (!(p.first == is_op && p.second == "else")) {
            //lastif.pop();
        }
    }
    rt->newNode(p);
    if (is_constant(p.first) || (p.first == is_var)) {
        return ;
    }
    if (p.first == is_func) {
        //if (p.second == "if") {
            //lastif.push(rt->last());
            ////rt->fa->lastif = rt->last();
        //}
        if (p.second == "else") {
            AST *curlastif = lastif.top();
            //AST *curlastif = rt->fa->lastif;
            rt->node.pop_back();
            curlastif->newNode({is_func, "_"});
            //rt->fa->fa->fa->fa->fa->fa->fa->fa->print(0);
            //getchar();
            bool has_brace = false;
            if (is_left_brace(st.top().second)) {
                has_brace = true;
            }
            if (has_brace) {
                st.pop();
            }
            if (has_brace) {
                while (!st.empty() && !(st.top().first == is_op && is_right_brace(st.top().second))) {
                    Build(curlastif->last(), st);
                }
            } else {
                Build(curlastif->last(), st);
            }
            if (has_brace) {
                st.pop();
            }
        } else if (p.second == "break" || p.second == "continue") {
            //rt->fa->fa->fa->fa->fa->fa->fa->fa->print(0);
        } else {
            st.pop();
            while (!st.empty() && !(st.top().first == is_op && is_right_brace(st.top().second))) {
                Build(rt->last(), st);
            }
            st.pop();
        }
        if (keywords[p.second]) {
            Build(rt->last(), st);
        }
        if (p.second == "if") {
            if (!st.empty() && ((st.top().first == is_func || st.top().first == is_var) && st.top().second == "else")) {
                lastif.push(rt->last());
            }
        }
        if (p.second == "func") {
            rt->last()->last()->newNode({is_func, "_"});
            rt->last()->last()->last()->endpoint = true;
        }
        return ;
    }
    if (p.first == is_op) {
        Build(rt->last(), st);
        if (p.second != "!" && p.second != "~") {
            Build(rt->last(), st);
        }
        return ;
    }
    if (st.top().first != is_op || is_left_brace(st.top ().second)) {
        Error("Not a function.");
    }
    //st.pop();
    while (st.top().first != is_op && is_right_brace(st.top().second)) {
        Build(rt->last(), st);
    }
    //st.pop();
}

map<string, int> priority = {
    {"=", 0},
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
};

AST* root = new AST({is_func, "_"});

void Gos::ClearGos() {
    if (root != nullptr) {
        delete root;
    }
    root = new AST({is_func, "_"});
}

void Gos::ImportConioLib() {
    root->addConst("BLACK", BLACK);
    root->addConst("BLUE", BLUE);
    root->addConst("GREEN",  GREEN);
    root->addConst("CYAN", CYAN);
    root->addConst("RED", RED);
    root->addConst("PURPLE", RED);
    root->addConst("YELLOW", YELLOW);
    root->addConst("WHITE", WHITE);
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

int Gos::RunGos(char filename[]) {
    if (filename != nullptr && strlen(filename) != 0) {
        fp = fopen(filename, "r");
    } else {
        fp = stdin;
    }
    srand(time(NULL));
    stack<pair<int, string>> result, tmp, orig;
    while (!end_of_file) {
        orig.push(get_token());
    }
    //tmp.push({is_func, "_"});
    while (!orig.empty()) {
        auto p = orig.top();
        orig.pop();
        if (p.first == is_op && (p.second == ";"  || p.second == ",")) {
            while (!tmp.empty() && !(tmp.top().first == is_op && is_right_brace(tmp.top().second))) {
                result.push(tmp.top());
                tmp.pop();
            }
            continue;
        }
        if (is_constant(p.first) || p.first == is_var) {
            result.push(p);
        } else if (p.first == is_op && is_right_brace(p.second)) {
            result.push(p);
            tmp.push(p);
        } else if ((p.first == is_op && is_left_brace(p.second)) || p.first == is_func) {
            while (!is_right_brace(tmp.top().second)) {
                result.push(tmp.top());
                tmp.pop();
            }
            result.push({is_op, "("});
            tmp.pop();
            if (p.first == is_func) {
                result.push(p);
            } else {
                result.push({is_func, "_"});
            }
        } else {
            while (true) {
                if (tmp.empty() || (tmp.top().first == is_op && is_right_brace(tmp.top().second)) || priority[p.second] >= priority[tmp.top().second]) {
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
    root->addConst("PI", 3.1415926535);
    root->addConst("E", 2.718281828459);
    root->addConst("true", 1);
    root->addConst("false", 0);
    root->addConst("NULL", 0);
    // Debug function
    funcs["printAST"] = [&](vector<Any> args) -> Any {
        root->print(0);
        return nullptr;
    };
    while (!result.empty()) {
        Build(root, result);
    }
    root->newNode({is_func, "_"});
    root->last()->endpoint = true;
    return root->Run().Int();
}

