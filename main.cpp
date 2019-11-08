#include "Gos.h"

int main(int argc, char* argv[]) {
    Gos::ImportConioLib();
    if (argc > 1) {
        Gos::RunGos(argv[1]);
    } else {
        Gos::RunGos(nullptr);
    }
    return 0;
}
