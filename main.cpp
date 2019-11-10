#include "Gos.h"

int main(int argc, char* argv[]) {
    srand(time(NULL));
    Gos::ImportDefaultLib();
    vector<Any> args;
    for (int i = 0; i < argc; i ++) {
        args.push_back((string)argv[i]);
    }
    if (argc > 1) {
        Gos::ExecuteFunc(Gos::BuildGos(argv[1]), {args});
    } else {
        Gos::ExecuteFunc(Gos::BuildGos(nullptr), {args});
    }
    return 0;
}
