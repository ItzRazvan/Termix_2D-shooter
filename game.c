#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <termios.h>
#include <sys/ioctl.h>


void start_screen();
void exit_game();

//--------------------------------------------------------------------------

#define TIMEOUT 20000
#define MOVE_DELAY 10

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

//--------------------------------------------------------------------

typedef struct{
    int x;
    int y;
} Coords;

typedef struct{
    int stage;
} Weapon;

typedef struct{
    Coords coords;
    bool is_alive;
    int health;
    Weapon weapon;        
} Shooter;

typedef struct{
    bool is_running;
    Shooter shooter;
    int kills;
    int best_game;
} Game;

Game game;

void game_init(){
    game.kills = 0;
    game.is_running = 1;
}

void shooter_init(){
    game.shooter.is_alive = 1;
    game.shooter.coords.x = window_width / 2;
    game.shooter.coords.y = window_height / 2;
    game.shooter.health = 100;
    game.shooter.weapon.stage = 1;
}

void elements_init(){
    game_init();
    shooter_init();
}

void print_shooter(){
    print_at(game.shooter.coords.x, game.shooter.coords.y, "x");
}

void print_elements(){
    print_shooter();
}

//--------------------------------------------------------------------

void leave_game(){
    game.is_running = 0;
    clear_terminal();
    start_screen();
}

void listen_for_input(char* key, int* move_counter){
    if(*move_counter >= MOVE_DELAY){
        if(*key == '\0')
            *key = getchar();

        if(*key != '\0'){
            switch (*key){
            case 'w':
                game.shooter.coords.y--;
                break;
            case 'a':
                game.shooter.coords.x--;
                break;
            case 's':
                game.shooter.coords.y++;
                break;
            case 'd':
                game.shooter.coords.x++;
                break;
            case 27:
                leave_game();
                break;
            default:
                break;
            }

            *key = '\0';
            *move_counter = 0;
        }
    }
}

//--------------------------------------------------------------------

void game_loop(){
    char key = '\0';
    int move_counter = 0;
    while(game.is_running){
        clear_terminal();
        print_elements();

        listen_for_input(&key, &move_counter);
        move_counter = (move_counter + 1) % 1000;

        usleep(TIMEOUT);
    }
}

//--------------------------------------------------------------------

void exit_game(){
    clear_terminal();
    printf("\033[?25h");  //show cursor
    exit(1);
}

void start_game(){
    clear_terminal();
    elements_init();
    game_loop();
}

void start_screen(){
    print_at(window_width/8, window_height/3-3, "Press 's' to start the game");
    print_at(window_width/8, window_height/3-1, "Press 'q' to quit  the game");


    while(1){
        char key = getchar();

        if(key == 'q'){
            exit_game();
        }else if(key == 's'){
            start_game();
        }

        usleep(TIMEOUT);
    }
}

//-----------------------------------------------------------------------

int main(){
    terminal_init();

    start_screen();

    return 0;
}
