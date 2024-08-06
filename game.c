#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <termios.h>
#include <sys/ioctl.h>

#define TIMEOUT 80000

//--------------------------------------------------------------------------
int window_height;
int window_width;

struct termios orig_terminal;
void disable_raw_mode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_terminal);
}

void enable_raw_mode(){
    tcgetattr(STDIN_FILENO, &orig_terminal);
    atexit(disable_raw_mode);

    struct termios raw = orig_terminal;

    raw.c_lflag &= ~(ECHO | ICANON );

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void get_terminal_size(){
    struct winsize size;
    ioctl(0, TIOCGWINSZ, &size);
    window_height = size.ws_row;
    window_width = size.ws_col;
}

//------------------------------------------------------------------

void print_at(int x, int y, char* c){
    printf("\033[%d;%dH%s", y, x, c);
}


void clear_terminal(){
    printf("\033[H\033[J");
}

void terminal_init(){
    enable_raw_mode();
    get_terminal_size();
    clear_terminal();
    printf("\033[?25l"); //hide cursor
}

void exit_game(){
    clear_terminal();
    printf("\033[?25h");  //show cursor
    exit(1);
}



void start_game(){
    elements_init();
}

void start_screen(){
    while(1){
        print_at(window_width/8, window_height/3-3, "Press 's' to start the game");
        print_at(window_width/8, window_height/3-1, "Press 'q' to quit  the game");

        char key = getchar();

        if(key == 'q'){
            exit_game();
        }else if(key == 's'){
            start_game();
        }

        usleep(TIMEOUT);
    }
}

int main(){
    terminal_init();

    start_screen();

    return 0;
}
