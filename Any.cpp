#include "Any.h"

#define Output(Type)                    \
    if (a.x->type() == typeid(Type)) {  \
        os << any_cast<Type>(*a.x);     \
    }

ostream& operator << (ostream& os, const Any& a) {
    Output(int) else Output(double) else Output(string) else if (a.x->type() == typeid(vector<Any>)) {
        os << '{';
        const vector<Any>& vec = any_cast<vector<Any>>(*a.x);
        if (vec.size() == 0) {
            os << '}';
            return os;
        }
        for (int i = 0; i < vec.size() - 1; ++ i) {
            os << vec[i] << ", ";
        }
        os << vec[vec.size() - 1] << '}';
    }
    return os;
}

istream& operator >> (istream& is, Any& a) {
    string s;
    cin >> s;
    bool all_digit = true;
    int has_dot = 0;
    for (const char& ch : s) {
        if (!isdigit(ch) && ch != '.') {
            all_digit = false;
            break;
        }
        if (ch == '.') {
            has_dot ++;
        }
    }
    if (!all_digit || has_dot > 1) {
        a = s;
        return is;
    }
    stringstream ss;
    ss << s;
    if (all_digit && has_dot == 0) {
        int tmp;
        ss >> tmp;
        a = tmp;
    } else {
        double tmp;
        ss >> tmp;
        a = tmp;
    }
    return is;
}
#undef Output

#define Compare(Type, op, rettype)                                      \
    if (x->type() == typeid(Type)) {                                    \
        return (rettype)(any_cast<Type>(*x) op any_cast<Type>(*val.x)); \
    }

void Any::SetConst() {
    isconst = true;
}

void Any::SetVar() {
    isconst = false;
    if (x->type() == typeid(vector<Any>)) {
        int len = GetSize();
        for (int i = 0; i < len; i ++) {
            (*this)[i].SetVar();
        }
    }
}

void Any::SetFunction(Function func) {
    *x = func;
}

void Any::SetValue(const any& a) {
    *x = a;
}

bool Any::IsConst() {
    return isconst;
}

bool Any::HasValue() {
    return x->has_value();
}

#define DefGet(Type, FuncName, TypeName)            \
Type Any::FuncName() const {                        \
    if (x->type() != typeid(Type)) {                \
        Error((string)"Not a " + TypeName + ".");   \
    }                                               \
    return any_cast<Type>(*x);                      \
}

DefGet(Function, Func, "function")
DefGet(int, Int, "integer");
DefGet(double, Double, "double");
DefGet(string, String, "string");
#undef DefGet

const type_info& Any::GetType() {
    return x->type();
}

const bool Any::operator == (const Any& val) const {
    if (x->type() != val.x->type()) {
        return false;
    }
    Compare(int, ==, int) else Compare(double, ==, int) else Compare(string, ==, int) else if (x->type() == typeid(vector<Any>)) {
        if (any_cast<vector<Any>>(*x).size() != any_cast<vector<Any>>(*val.x).size()) {
            return false;
        }
        for (int i = 0; i < any_cast<vector<Any>>(*x).size(); ++ i) {
            if (any_cast<vector<Any>>(*x)[i] != any_cast<vector<Any>>(*val.x)[i]) {
                return false;
            }
        }
    } else {
        Warning((string)"Unable to compare: " + (string)x->type().name() + (string)" and " + (string)val.x->type().name());
        return false;
    }
    return true;
}

const bool Any::operator != (const Any& val) const {
    return !(*this == val);
}

const bool Any::operator < (const Any& val) const {
    if (x->type() != val.x->type()) {
        Warning((string)"Unable to compare: " + (string)x->type().name() + (string)" and " + (string)val.x->type().name());
        return false;
    }
    Compare(int, <, int) else Compare(double, <, int) else Compare(string, <, int) else if (x->type() == typeid(vector<Any>)) {
        bool issame = true;
        for (int i = 0; i < min(any_cast<vector<Any>>(*x).size(), any_cast<vector<Any>>(*val.x).size()); ++ i) {
            if (any_cast<vector<Any>>(*x)[i] < any_cast<vector<Any>>(*val.x)[i]) {
                return true;
            } else if (any_cast<vector<Any>>(*x)[i] > any_cast<vector<Any>>(*val.x)[i]) {
                return false;
            }
        }
        return any_cast<vector<Any>>(*x).size() < any_cast<vector<Any>>(*val.x).size();
    } else {
        Warning((string)"Unable to compare: " + (string)x->type().name() + (string)" and " + (string)val.x->type().name());
        return false;
    }
}

const bool Any::operator <= (const Any& val) const {
    return (*this < val) || (*this == val);
}

const bool Any::operator >= (const Any& val) const {
    return !(*this < val);
}

const bool Any::operator > (const Any& val) const {
    return !(*this <= val);
}

Any Any::operator + (const Any& val) const {
    if (x->type() != val.x->type()) {
        Warning((string)"Unable to calculate: " + (string)x->type().name() + (string)" + " + (string)val.x->type().name());
        return 0;
    }
    Compare(int, +, int) else Compare(double, +, double) else Compare(string, +, string) else if (x->type() == typeid(vector<Any>)) {
        vector<Any> ans;
        for (const Any& a : any_cast<vector<Any>>(*x)) {
            ans.push_back(a);
        }
        for (const Any& a : any_cast<vector<Any>>(*val.x)) {
            ans.push_back(a);
        }
        return ans;
    } else {
        Warning((string)"Unable to calculate: " + (string)x->type().name() + (string)" + " + (string)val.x->type().name());
        return 0;
    }
}

#define DefOp(op, opstr)\
Any Any::operator op (const Any& val) const {\
    if (x->type() != val.x->type()) {\
        Warning((string)"Unable to calculate: " + (string)x->type().name() + " " + (string)opstr + " " + (string)val.x->type().name());\
        return 0;\
    }\
    Compare(int, op, int) else Compare(double, op, double) else {\
        Warning((string)"Unable to calculate: " + (string)x->type().name() + " " + (string)opstr + " " + (string)val.x->type().name());\
        return 0;\
    }\
}

DefOp(-, "-");
DefOp(*, "*");
DefOp(/, "/");
#undef DefOp

Any Any::Pow (const Any& val) {
    if (x->type() != val.x->type()) {
        Warning((string)"Unable to calculate: " + (string)x->type().name() + (string)" ** " + (string)val.x->type().name());
        return 0;
    }
    if (x->type() == typeid(int)) {
        return (int)pow((double)any_cast<int>(*x), (double)any_cast<int>(*val.x));
    } else if (x->type() == typeid(double)) {
        return (double)pow(any_cast<double>(*x), any_cast<double>(*val.x));
    } else {
        Warning((string)"Unable to calculate: " + (string)x->type().name() + (string)" ** " + (string)val.x->type().name());
        return 0;
    }
}

Any& Any::Assign(Any& val) {
    if (isconst) {
        Warning("Unable to change a const");
        return *this;
    }
    if (val.x->type() == typeid(map<string, Any>)) {
        *x = any(map<string, Any>());
        map<string, Any>& cur = *this->cast<map<string, Any>>();
        map<string, Any>& other = *val.cast<map<string, Any>>();
        for (auto& x : other) {
            cur[x.first] = 0;
            cur[x.first].Assign(x.second);
        }
    } else if (val.x->type() == typeid(vector<Any>)) {
        *x = vector<Any>();
        int len = val.GetSize();
        for (int i = 0; i < len; i ++) {
            PushBack(0);
            (*this)[i].Assign(val[i]);
        }
    } else {
        *x = *val.x;
    }
    SetVar();
    return *this;
}

shared_ptr<any> Any::getRef() {
    return x;
}

Any& Any::LinkTo(shared_ptr<any> val) {
    if (isconst) {
        Warning("Unable to change a const");
        return *this;
    }
    x = val;
    isconst = false;
    return *this;
}

Any& Any::UnLink() {
    x = make_shared<any>(new any());
    isconst = false;
    return *this;
}

#undef Compare

void Any::PushBack(Any val) {
    if (x->type() != typeid(vector<Any>)) {
        Error("Not a vector.");
    }
    cast<vector<Any>>()->push_back(val);
}

void Any::PopBack() {
    if (x->type() != typeid(vector<Any>)) {
        Error("Not a vector.");
    }
    cast<vector<Any>>()->pop_back();
}

void Any::EraseAt(int p) {
    if (x->type() != typeid(vector<Any>)) {
        Error("Not a vector.");
    }
    cast<vector<Any>>()->erase(cast<vector<Any>>()->begin() + p);
}

int Any::GetSize() const {
    if (x->type() != typeid(vector<Any>)) {
        Error("Not a vector.");
    }
    return any_cast<vector<Any>>(*x).size();
}

Any& Any::operator [] (const int& id) {
    if (x->type() != typeid(vector<Any>)) {
        Error("Not a vector.");
    }
    return (*cast<vector<Any>>())[id];
}

Any& Any::operator [] (const vector<Any>& ids) {
    Any *ans = this;
    for (int i = 0; i < ids.size(); i ++) {
        if (ans->x->type() != typeid(vector<Any>)) {
            Error("Not a vector.");
        }
        ans = &(*ans->cast<vector<Any>>())[ids[i].Int()];
    }
    return *ans;
}

Any repeat(Any orig, int cnt) {
    vector<Any> ret;
    for (int i = 0; i < cnt; i ++) {
        ret.push_back(0);
        ret[i].Assign(orig);
    }
    Any tmp = ret;
    tmp.SetConst();
    return tmp;
}

Any Any::Repeat(const vector<Any>& ids) {
    Any ret(0);
    for (int i = ids.size() - 1; i >= 0; i --) {
        ret = repeat(ret, ids[i].Int());
    }
    return ret;
}

