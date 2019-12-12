#ifndef CONIOPLUS_HPP
#define CONIOPLUS_HPP

#if defined(linux) || defined(__APPLE__)

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <cstdio>
#include <cstdlib>

const int BLACK = 30;
const int RED = 31;
const int GREEN = 32;
const int YELLOW = 33;
const int BLUE = 34;
const int MAGENTA = 35;
const int CYAN = 36;
const int LIGHT_GRAY = 37;
const int DARK_GRAY = 90;
const int LIGHT_RED = 91;
const int LIGHT_GREEN = 92;
const int LIGHT_YELLOW = 93;
const int LIGHT_BLUE = 94;
const int LIGHT_MAGENTA = 95;
const int LIGHT_CYAN = 96;
const int WHITE = 97;

bool kbhit();
int getch();

#else

#include <conio.h>
#include <windows.h>
#include <time.h>

const int BLACK = 0;
const int RED = 4;
const int GREEN = 2;
const int YELLOW = 6;
const int BLUE = 1;
const int MAGENTA = 5;
const int CYAN = 3;
const int LIGHT_GRAY = 15;
const int DARK_GRAY = 8;
const int LIGHT_RED = 12;
const int LIGHT_GREEN = 10;
const int LIGHT_YELLOW = 14;
const int LIGHT_BLUE = 9;
const int LIGHT_MAGENTA = 13;
const int LIGHT_CYAN = 11;
const int WHITE = 7;

#define kbhit _kbhit
#define getch _getch

#endif

void gotoxy(int x, int y);
void hidecursor();
void showcursor();
void color(int a, int b);
void clearcolor();
void clearscreen();
double gettime();
int readkey(double limit_time);
#endif
