#include "Error.h"
#include <iostream>
using namespace std;

void __Error(string msg) {
    color(RED, BLACK);
    cout << "Error: ";
    clearcolor();
    cout << msg << endl;
}

void __Warning(string msg) {
    color(MAGENTA, BLACK);
    cout << "Warning: ";
    clearcolor();
    cout << msg << endl;
}
