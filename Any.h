#ifndef HEPTAGON196_GOS_ANY_H
#define HEPTAGON196_GOS_ANY_H
#include <iostream>
#include <sstream>
#include <any>
#include <string>
#include <vector>
#include <cmath>
#include <functional>
#include "Error.h"
using namespace std;
class Any {
    friend ostream& operator << (ostream&, const Any& a);
    friend istream& operator >> (istream&, Any& a);
    private:
        any *x = new any;
        bool isconst;
        bool islink = false;
    public:
        Any() : isconst(false) {};
        template<typename T> Any(const T& val) : isconst(false) { *x = val; };
        template<typename T> Any& operator = (const T& val);
        template<typename T> T* cast() {
            return any_cast<T>(x);
        }
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
        const bool operator == (const Any& val);
        const bool operator != (const Any& val);
        const bool operator < (const Any& val);
        const bool operator > (const Any& val);
        const bool operator <= (const Any& val);
        const bool operator >= (const Any& val);
        Any operator + (const Any& val);
        Any operator - (const Any& val);
        Any operator * (const Any& val);
        Any operator / (const Any& val);
        Any Pow(const Any& val);
        Any& Assign(Any& val);
        Any& LinkTo(Any& val);
        //support for vector
        void PushBack(Any val);
        void PopBack();
        int GetSize() const;
        Any& operator [] (const int& id);
        Any& operator [] (const vector<Any>& ids);
        Any Repeat(const vector<Any>& ids);
};
typedef function<void(vector<Any*>)> Function;
ostream& operator << (ostream& os, const Any& a);

template<typename T>Any& Any::operator = (const T& val) {
    if (isconst) {
        Warning("Unable to change a const");
        return *this;
    }
    *x = val;
    SetVar();
    return *this;
}

#undef Output
#endif
