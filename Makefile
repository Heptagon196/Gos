CXX=g++ --std=c++20
DEFAULT: Reflection/reflection.o main.o GosVM.o GosAST.o GosTokenizer.o Gos.o
	$(CXX) Reflection/reflection.o GosVM.o GosTokenizer.o GosAST.o Gos.o main.o -o gos
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
Gos.o: Gos.cpp
	$(CXX) -c Gos.cpp
GosTokenizer.o: GosTokenizer.cpp
	$(CXX) -c GosTokenizer.cpp
