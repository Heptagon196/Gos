FLAG = --std=c++17 -Dlinux -O2
CPP = g++
DEPENDENCIES = ./Any.o ./main.o ./ConioPlus.o ./Error.o ./Gos.o

calculator: $(DEPENDENCIES)
	$(CPP) $(DEPENDENCIES) $(FLAG) -o gos

debug: $(DEPENDENCIES)
	$(CPP) $(DEPENDENCIES) $(FLAG) -o gos -DDEBUG -g

install:
	cp ./gos /usr/local/bin/gos

./Any.o: ./Any.cpp
	$(CPP) -c $(FLAG) $< -o ./Any.o
./main.o: ./main.cpp
	$(CPP) -c $(FLAG) $< -o ./main.o
./ConioPlus.o: ./ConioPlus.cpp
	$(CPP) -c $(FLAG) $< -o ./ConioPlus.o
./Error.o: ./Error.cpp
	$(CPP) -c $(FLAG) $< -o ./Error.o
./Gos.o: ./Gos.cpp
	$(CPP) -c $(FLAG) $< -o ./Gos.o

clean:
	rm -rf *.o
