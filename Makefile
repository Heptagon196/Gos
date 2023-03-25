DEFAULT: Reflection/reflection.o
	g++ --std=c++20 *.cpp Reflection/reflection.o -o gos
Reflection/reflection.o:
	cd Reflection&&make link
