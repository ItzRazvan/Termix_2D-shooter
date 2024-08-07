#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>

void start_screen();
void exit_game();

//--------------------------------------------------------------------------

#define MOVE_COOLDOWN 4

#define SHOOT_COOLDOWN_WEAPON_1 10

#define UP 1
#define RIGHT 2
#define DOWN 3
#define LEFT 4

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
    Coords coords;
    int direction;
} Bullet;

typedef struct{
    int stage;
    int cooldown;
    Bullet bullets[200];
} Weapon;

typedef struct{
    Coords coords;
    bool is_alive;
    int health;
    Weapon weapon;   
    int shooting_dir;     
} Shooter;

typedef struct{
    bool is_running;
    Shooter shooter;
    int kills;
    int best_game;
    int bullet_count;
} Game;

Game game;

void game_init(){
    game.kills = 0;
    game.is_running = 1;
    game.bullet_count = 0;
}

void weapon_init(){
    game.shooter.weapon.stage = 1;
    game.shooter.weapon.cooldown = SHOOT_COOLDOWN_WEAPON_1;
}

void shooter_init(){
    game.shooter.is_alive = 1;
    game.shooter.coords.x = window_width / 2;
    game.shooter.coords.y = window_height / 2;
    game.shooter.health = 100;
    game.shooter.shooting_dir = RIGHT;
    weapon_init();
}

void elements_init(){
    game_init();
    shooter_init();
}

void print_shooter(){
    print_at(game.shooter.coords.x, game.shooter.coords.y, "x");
}

void print_bullets(){
    for(int i = 0; i < game.bullet_count; ++i)
        print_at(game.shooter.weapon.bullets[i].coords.x, game.shooter.weapon.bullets[i].coords.y, ".");
}

void print_elements(){
    print_shooter();
    print_bullets();
}

//--------------------------------------------------------------------


int movement;
void set_movement(){
    movement = (movement + 1) % 1000000;
}

int shoot_cooldown;
void set_shoot_cooldown(){
    shoot_cooldown = (shoot_cooldown + 1) % 1000000;
}

//--------------------------------------------------------------------

void bullet_init(int index){
    game.shooter.weapon.bullets[index].direction = game.shooter.shooting_dir;
    switch (game.shooter.shooting_dir){
    case UP:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y - 1;
        break;
    case RIGHT:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x + 1;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y;
        break;
    case DOWN:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y + 1;
        break;
    case LEFT:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x - 1;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y;
        break;
    
    default:
        break;
    }
}

void shoot_weapon_1(){
    bullet_init(game.bullet_count);
    shoot_cooldown = 0;
    game.bullet_count++;
}

void shoot(){
    if(shoot_cooldown >= game.shooter.weapon.cooldown){
        switch (game.shooter.weapon.stage){
        case 1:
            shoot_weapon_1();
            break;
        
        default:
            break;
        }
    }
}

void move_bullets(){
    for(int i = 0; i < game.bullet_count; ++i){
        switch (game.shooter.weapon.bullets[i].direction){
        case UP:
            game.shooter.weapon.bullets[i].coords.y--;
            break;
        case RIGHT:
            game.shooter.weapon.bullets[i].coords.x++;
            break;
        case DOWN:
            game.shooter.weapon.bullets[i].coords.y++;
            break;
        case LEFT:
            game.shooter.weapon.bullets[i].coords.x--;
            break;
        
        default:
            break;
        }
    }
}

//--------------------------------------------------------------------

void leave_game(){
    game.is_running = 0;
    clear_terminal();
    start_screen();
}

void handle_key(char* key){
    if(*key == 27){
        leave_game();
        return;
    }

    if(*key == 'k'){
        shoot();
        return;
    }

    if(movement >= MOVE_COOLDOWN){
        movement = 0;
        switch (*key){
                case 'w':
                    game.shooter.coords.y--;
                    game.shooter.shooting_dir = UP;
                    break;
                case 'a':
                    game.shooter.coords.x--;
                    game.shooter.shooting_dir = LEFT;
                    break;
                case 's':
                    game.shooter.coords.y++;
                    game.shooter.shooting_dir = DOWN;
                    break;
                case 'd':
                    game.shooter.coords.x++;
                    game.shooter.shooting_dir = RIGHT;
                    break;
                default:
                    break;
                }
    }
}


void listen_for_input(char* key){
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(STDIN_FILENO, &fd);

    struct timeval timeint;
    timeint.tv_sec = 0;
    timeint.tv_usec = 30000;

    int res = select(STDIN_FILENO + 1, &fd, NULL, NULL, &timeint);

    if(res > 0){
        read(STDIN_FILENO, key, 1);
        if(game.is_running)
            handle_key(key);
    }
}

//--------------------------------------------------------------------

void game_loop(){
    char key = '\0';
    movement = 0;
    while(game.is_running){
        fflush(stdout);
        clear_terminal();
        print_elements();

        set_movement();
        set_shoot_cooldown();

        move_bullets();

        listen_for_input(&key);
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

    char key = getchar();
        
    if(key == 'q'){
        exit_game();
    }else if(key == 's'){
        start_game();
    }
    
}

//-----------------------------------------------------------------------

int main(){
    terminal_init();

    start_screen();

    return 0;
}
