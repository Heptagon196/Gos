// bool kbhit(): 是否有键按下
// int getch(): 读取用户按下的键
// void gotoxy(int x, int y): 移动光标位置
// void hidecursor(): 隐藏光标
// void showcursor(): 显示光标
// void color(int a, int b): 改变以后输出字符的前景色和后景色
// void clearcolor(): 恢复默认前景色和后景色
// void clearscreen(): 清屏
// double gettime(): 获取时间，单位秒
// int readkey(double limit_time): 获取用户在规定时间(单位秒)内按下的键，超时返回 0
#include "ConioPlus.h"
#if defined(linux) || defined(__APPLE__)

bool kbhit() {
	struct termios oldt, newt;
	int ch;
	int oldf;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	if(ch != EOF) {
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}

int getch() {
     struct termios tm, tm_old;
     int fd = 0, ch;
     if (tcgetattr(fd, &tm) < 0) {//保存现在的终端设置
          return -1;
     }
     tm_old = tm;
     cfmakeraw(&tm);//更改终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
     if (tcsetattr(fd, TCSANOW, &tm) < 0) {//设置上更改之后的设置
          return -1;
     }
     ch = getchar();
     if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {//更改设置为最初的样子
          return -1;
     }
     return ch;
}

void gotoxy(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

void hidecursor() {
	printf("\033[?25l");
}

void showcursor() {
	printf("\033[?25h");
}

void color(int a, int b) {
    printf("\033[%dm\033[%dm", b + 40, a + 30);
}

void clearcolor() {
    printf("\033[0m");
}

void clearscreen() {
    system("clear");
}

#else

#define __CONIOPLUS_COLOR(x, y) (x + 16 * y)

void gotoxy(int x, int y) {
	COORD c;
	c.X = x - 1;
	c.Y = y - 1;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void hidecursor() {
	CONSOLE_CURSOR_INFO cursor_info = {1, 0};
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}

void showcursor() {
	CONSOLE_CURSOR_INFO cursor_info = {1, 25};
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}

void color(int a, int b) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), __CONIOPLUS_COLOR(a,b));
}

void clearcolor() {
    color(WHITE, BLACK);
}

void clearscreen() {
    system("cls");
}

#endif

double gettime() {
	return (double)clock() / CLOCKS_PER_SEC;
}

int readkey(double limit_time)
{
	double prev_time = gettime();
	while (!kbhit() && (gettime() - prev_time < limit_time)) {};
    int ret = (kbhit()) ? getch() : 0;
    while (gettime() - prev_time < limit_time) {
        if (kbhit()) {
            getch();
        }
    };
    while (kbhit()) {
        getch();
    }
    return ret;
}
