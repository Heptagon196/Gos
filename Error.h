#ifndef HEPTAGON196_GOS_ERROR_H
#define HEPTAGON196_GOS_ERROR_H
#include <string>
#include <sstream>
#include "ConioPlus.h"

#define LINEINFO ("Line " + ([](int i) -> string { stringstream ss; string s; ss << i; ss >> s; return s; })(__LINE__) + ": ")
void __Error(std::string msg);
void __Warning(std::string msg);
#define Error(s) __Error((s))
#define Warning(s) __Warning((s))

#endif
