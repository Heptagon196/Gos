.SILENT:
FLAG = --std=c++17 -Dlinux
CPP = g++
DEPENDENCIES = ./Gos.o ./Error.o ./Any.o ./main.o ./ConioPlus.o
ifdef MAKE_RELEASE
	FLAG += -O2
endif
ifdef MAKE_DEBUG
	FLAG += -DDEBUG -g
endif
ifdef MAKE_STATIC
	FLAG += -static
endif

default:
	make release

gos: $(DEPENDENCIES)
	 echo "[6/6] Building gos"
	$(CPP) $(DEPENDENCIES) $(FLAG) -o gos

./Gos.o: ./Gos.cpp
	echo "[1/6] Compiling ./Gos.cpp"
	$(CPP) -c $(FLAG) $< -o ./Gos.o
./Error.o: ./Error.cpp
	echo "[2/6] Compiling ./Error.cpp"
	$(CPP) -c $(FLAG) $< -o ./Error.o
./Any.o: ./Any.cpp
	echo "[3/6] Compiling ./Any.cpp"
	$(CPP) -c $(FLAG) $< -o ./Any.o
./main.o: ./main.cpp
	echo "[4/6] Compiling ./main.cpp"
	$(CPP) -c $(FLAG) $< -o ./main.o
./ConioPlus.o: ./ConioPlus.cpp
	echo "[5/6] Compiling ./ConioPlus.cpp"
	$(CPP) -c $(FLAG) $< -o ./ConioPlus.o

clean:
	echo "Cleaning files"
	rm -rf ./Gos.o ./Error.o ./Any.o ./main.o ./ConioPlus.o
install:
	echo "Installing gos to /usr/local/bin"
	cp ./gos /usr/local/bin/
debug:
	echo "Building a debug version"
	env MAKE_DEBUG=true make gos
static:
	echo "Building a statically linked version"
	env MAKE_STATIC=true make gos
release:
	echo "Building a release version"
	env MAKE_RELEASE=true make gos
