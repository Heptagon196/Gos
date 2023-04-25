CXX=g++ --std=c++20 -O2
DEFAULT: Reflection/reflection.o main.o GosVM.o GosAST.o GosASTCompiler.o GosTokenizer.o Gos.o GosFileSystem.o
	$(CXX) Reflection/reflection.o GosVM.o GosTokenizer.o GosAST.o GosASTCompiler.o Gos.o GosFileSystem.o main.o -o gos
clean:
	rm *.o
Reflection/reflection.o:
	cd Reflection&&make link
main.o: main.cpp
	$(CXX) -c main.cpp
GosVM.o: GosVM.cpp
	$(CXX) -c GosVM.cpp
GosAST.o: GosAST.cpp
	$(CXX) -c GosAST.cpp
GosASTCompiler.o: GosASTCompiler.cpp
	$(CXX) -c GosASTCompiler.cpp
Gos.o: Gos.cpp
	$(CXX) -c Gos.cpp
GosFileSystem.o: GosFileSystem.cpp
	$(CXX) -c GosFileSystem.cpp
GosTokenizer.o: GosTokenizer.cpp
	$(CXX) -c GosTokenizer.cpp
