#ifndef HEPTAGON196_GOS_ANY_H
#define HEPTAGON196_GOS_ANY_H
#include <iostream>
#include <sstream>
#include <any>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <functional>
#include <memory>
#include "Error.h"

using namespace std;
class Any {
    friend ostream& operator << (ostream&, const Any& a);
    friend istream& operator >> (istream&, Any& a);
    private:
        shared_ptr<any> x = make_shared<any>(new any());
        bool isconst;
    public:
        Any() : isconst(false) {};
        template<typename T> Any(const T& val) : isconst(false) { *x = val; };
        Any(shared_ptr<any> x) : x(x) {}
        template<typename T> Any& operator = (const T& val);
        template<typename T> T* cast() {
            return any_cast<T>(x.get());
        }
        shared_ptr<any> getRef();
        void SetConst();
        void SetVar();
        void SetFunction(function<void(vector<Any*> args)> func);
        void SetValue(const any& a);
        bool IsConst();
        bool HasValue();
        const type_info& GetType();
        function<void(vector<Any*>)> Func() const;
        int Int() const;
        double Double() const;
        string String() const;
        const bool operator == (const Any& val) const;
        const bool operator != (const Any& val) const;
        const bool operator < (const Any& val) const;
        const bool operator > (const Any& val) const;
        const bool operator <= (const Any& val) const;
        const bool operator >= (const Any& val) const;
        Any operator + (const Any& val) const;
        Any operator - (const Any& val) const;
        Any operator * (const Any& val) const;
        Any operator / (const Any& val) const;
        Any Pow(const Any& val);
        Any& Assign(Any& val);
        Any& LinkTo(shared_ptr<any> val);
        Any& UnLink();
        //support for vector
        void PushBack(Any val);
        void PopBack();
        void EraseAt(int p);
        int GetSize() const;
        Any& operator [] (const int& id);
        Any& operator [] (const vector<Any>& ids);
        Any Repeat(const vector<Any>& ids);
};
typedef function<void(vector<Any*>)> Function;
ostream& operator << (ostream& os, const Any& a);

template<typename T>Any& Any::operator = (const T& val) {
    if (isconst) {
        __Warning("Unable to change a const");
        return *this;
    }
    *x = val;
    SetVar();
    return *this;
}

#undef Output
#endif
