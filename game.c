#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <termios.h>
#include <sys/ioctl.h>

int window_height;
int window_width;

struct winsize default;
void disable_raw_mode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &default);
}

void enable_raw_mode(){
    tcgetattr(STDIN_FILENO, TCSAFLUSH, &default);
    atexit(disable_raw_mode);

    struct termios raw = default;

    raw.c_flag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void get_terminal_size(){
    struct winsize size;
    ioctl(0, TIOCGWINSZ, &size);
    screen_height = size.ws_row;
    screen_width = size.ws_col;
}

void terminal_init(){
    enable_raw_mode();
    get_terminal_size();
}

int main(){
    terminal_init();
}
