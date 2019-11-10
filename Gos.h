#ifndef HEPTAGON196_GOS_GOS_H
#define HEPTAGON196_GOS_GOS_H
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <stack>
#include <map>
#include <set>
#include <functional>
#include <fstream>
#include "Any.h"
namespace Gos {
    void ClearGos();
    void ImportDefaultLib();
    void ImportConst(map<string, Any> consts);
    void ImportVar(map<string, Any> vars);
    void ImportFunc(map<string, function<Any(vector<Any>)>> func);
    Any& GetVar(string name);
    Any ExecuteFunc(Any func, vector<Any> args);
    Any BuildGos(const char filename[]);
}
#endif
