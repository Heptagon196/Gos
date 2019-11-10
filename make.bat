@echo off
echo Building a release version
echo [1/6] Compiling ./Gos.cpp
g++ -c --std=c++17 -Dwindows -O2 ./Gos.cpp -o ./Gos.o
echo [2/6] Compiling ./Error.cpp
g++ -c --std=c++17 -Dwindows -O2 ./Error.cpp -o ./Error.o
echo [3/6] Compiling ./Any.cpp
g++ -c --std=c++17 -Dwindows -O2 ./Any.cpp -o ./Any.o
echo [4/6] Compiling ./main.cpp
g++ -c --std=c++17 -Dwindows -O2 ./main.cpp -o ./main.o
echo [5/6] Compiling ./ConioPlus.cpp
g++ -c --std=c++17 -Dwindows -O2 ./ConioPlus.cpp -o ./ConioPlus.o
echo [6/6] Building gos
g++ --std=c++17 -Dwindows -O2 ./Gos.o ./Error.o ./Any.o ./main.o ./ConioPlus.o -o gos
