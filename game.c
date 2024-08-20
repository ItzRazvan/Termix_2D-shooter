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
void start_game();
void update_best_score();

//--------------------------------------------------------------------------
#define MAX_BULLET_COUNT 1000
#define MAX_ENEMY_COUNT 30

#define Y_AXES_MOVEMENT_COOLDOWN 6
#define Y_AXES_BULLETS_COOLDOWN 3

#define Y_AXES 10
#define X_AXES 11

#define RED_COOLDOWN 15

#define MOVE_COOLDOWN 4
#define ENEMY_SPAWN_COOLDOWN 125

#define SHOOT_COOLDOWN_WEAPON_1 10
#define WEAPON_1_DAMAGE 10
#define HITBOX_1 1

#define SHOOT_COOLDOWN_WEAPON_2 30
#define WEAPON_2_DAMAGE 30
#define HITBOX_2 2

#define NR_OF_TYPES 1

#define ENEMY_TYPE_1 1
#define ENEMY_TYPE_1_HEALTH 15
#define ENEMY_TYPE_1_DAMAGE 5
#define ENEMY_TYPE_1_MOVE_COOLDOWN 16
#define ENEMY_TYPE_1_HIT_COOLDOWN 30

#define STAGE_2_THRESHOLD 10

#define ENEMY_TYPE_2 2
#define ENEMY_TYPE_2_HEALTH 40
#define ENEMY_TYPE_2_DAMAGE 15
#define ENEMY_TYPE_2_MOVE_COOLDOWN 12
#define ENEMY_TYPE_2_HIT_COOLDOWN 50

#define UP 1
#define RIGHT 2
#define DOWN 3
#define LEFT 4
#define D1 5
#define D2 6
#define D3 7
#define D4 8

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


void print_at_with_int(int x, int y, char* c, int i){
    printf("\033[%d;%dH%s%d", y, x, c, i);
}

void make_terminal_red(){
    printf("\033[41m");
}

void restore_terminal_colour(){
    printf("\033[0m");
}

void clear_terminal(){
    printf("\033[H\033[J");
}

void remove_terminal_indent() {
    printf("\033[?7l");
}

void restore_terminal_indent() {
    printf("\033[?7h");
}

void terminal_init(){
    enable_raw_mode();
    remove_terminal_indent();
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
    bool out_of_bounds;
    bool hit;
    int hitbox;
    int y_axes_count;
    int main_direction;
} Bullet;

typedef struct{
    int stage;
    int cooldown;
    Bullet bullets[MAX_BULLET_COUNT];
    int damage;
} Weapon;

typedef struct{
    Coords coords;
    int health;
    Weapon weapon;   
    int shooting_dir;  
    int y_axes_count;   
} Shooter;

typedef struct{
    Coords coords;
    int health;
    int damage;
    int type;
    bool alive;
    int move_cooldown;
    int count_move_cooldown;
    int hit_cooldown;
    int count_hit_cooldown;
    int y_axes_count;
} Enemy;

typedef struct{
    bool is_running;
    Shooter shooter;
    int score;
    int best_game;
    int bullet_count;
    Enemy enemies[MAX_ENEMY_COUNT];
    int enemy_count;
    int stage;
    bool red;
    int red_count;
} Game;

Game game;

void game_init(){
    game.score = 0;
    game.is_running = 1;
    game.bullet_count = 0;
    game.enemy_count = 0;
    game.stage = 1;
    game.red = 0;
    game.red_count = 0;
}

void weapon_init(){
    game.shooter.weapon.stage = 1;
    game.shooter.weapon.cooldown = SHOOT_COOLDOWN_WEAPON_1;
    game.shooter.weapon.damage = WEAPON_1_DAMAGE;
}

void shooter_init(){
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
        if(!game.shooter.weapon.bullets[i].out_of_bounds && !game.shooter.weapon.bullets[i].hit)
            print_at(game.shooter.weapon.bullets[i].coords.x, game.shooter.weapon.bullets[i].coords.y, ".");
}

void print_enemies(){
    for(int i = 0; i < game.enemy_count; ++i)
        if(game.enemies[i].alive)
            print_at(game.enemies[i].coords.x, game.enemies[i].coords.y, "0");

}

void print_stats(){
    print_at_with_int(2, window_height, "Health: ", game.shooter.health);
    print_at_with_int(15, window_height, "Your score is: ", game.score);
}

void print_elements(){
    print_bullets();
    print_enemies();
    print_shooter();
    print_stats();
}

void print_death_screen(){
    make_terminal_red();
    print_at(window_width/2 - 4, window_height/2 - 1, "YOU DIED");
    print_at_with_int(window_width/2-8, window_height/2 + 1, "Your score was: ", game.score);
    print_at(window_width/2 - 9, window_height/2 + 3, "Press r to restart");
    print_at(window_width/2 - 8, window_height/2 + 4, "Press m for menu");
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

int enemy_cooldown;
void set_enemy_spawn_cooldown(){
    enemy_cooldown = (enemy_cooldown + 1) % 1000000;
}

void set_enemies_move_cooldown(){
    for(int i = 0; i < game.enemy_count; ++i){
        game.enemies[i].count_move_cooldown = (game.enemies[i].count_move_cooldown + 1) % 1000000;
    }
}

void set_enemies_hit_cooldown(){
    for(int i = 0; i < game.enemy_count; ++i){
        game.enemies[i].count_hit_cooldown = (game.enemies[i].count_hit_cooldown + 1) % 1000000;
    }
}

void set_enemy_y_cooldown(){
    for(int i = 0; i < game.enemy_count; ++i){
        game.enemies[i].y_axes_count = (game.enemies[i].y_axes_count + 1) % 1000000;
    }
}

void set_shooter_y_cooldown(){
    game.shooter.y_axes_count = (game.shooter.y_axes_count + 1) % 1000000;
}

void set_bullets_y_cooldown(){
    for(int i = 0; i < game.bullet_count; ++i){
        game.shooter.weapon.bullets[i].y_axes_count = (game.shooter.weapon.bullets[i].y_axes_count + 1) % 1000000;
    }
}


void set_y_axes_cooldown(){
    set_enemy_y_cooldown();
    set_shooter_y_cooldown();
    set_bullets_y_cooldown();
}

void set_cooldowns(){
    set_movement();
    set_shoot_cooldown();
    set_enemy_spawn_cooldown();
    set_enemies_move_cooldown();
    set_enemies_hit_cooldown();
    set_y_axes_cooldown();
}

//--------------------------------------------------------------------

void check_bullet_collisions(int i){
    for(int j = 0; j < game.bullet_count; ++j){
        if(game.enemies[i].alive && !game.shooter.weapon.bullets[j].hit){
            bool hit = 0;

            switch (game.shooter.weapon.bullets[j].hitbox){
            case 1:
                if(game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y)
                    hit = 1;
                break;
            case 2:
                if((game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x + 1 && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x - 1 && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y + 1) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y - 1) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x + 1 && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y + 1) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x + 1 && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y - 1) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x - 1 && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y + 1) || (game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x - 1 && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y - 1))
                    hit = 1;
            default:
                break;
            }

            if(hit){
                game.enemies[i].health -= game.shooter.weapon.damage;
                game.shooter.weapon.bullets[j].hit = 1;
            }
        }
    }
    
}

void check_shooter_collisions(int i){
    if(game.enemies[i].alive && (game.enemies[i].coords.x == game.shooter.coords.x && game.enemies[i].coords.y == game.shooter.coords.y) || (game.enemies[i].coords.x == game.shooter.coords.x + 1 && game.enemies[i].coords.y == game.shooter.coords.y) || (game.enemies[i].coords.x == game.shooter.coords.x - 1 && game.enemies[i].coords.y == game.shooter.coords.y) || (game.enemies[i].coords.x == game.shooter.coords.x && game.enemies[i].coords.y == game.shooter.coords.y + 1) || (game.enemies[i].coords.x == game.shooter.coords.x && game.enemies[i].coords.y == game.shooter.coords.y - 1)){
        if(game.enemies[i].count_hit_cooldown >= game.enemies[i].hit_cooldown){
            game.shooter.health -= game.enemies[i].damage;
            make_terminal_red();
            game.red = 1;
            game.enemies[i].count_hit_cooldown = 0;
        }
    }
    
}

void check_dead(int i){
    if(game.enemies[i].alive && game.enemies[i].health <= 0){
        game.enemies[i].alive = 0;
        game.score++;
    }
}

void check_enemies(){
    for(int i = 0; i < game.enemy_count; ++i){
        if(game.enemies[i].alive){
            check_bullet_collisions(i);
            check_shooter_collisions(i);
            check_dead(i);
        }
    }
}

void enemy_init(int type, int index){
    int x = (rand() % window_width - 2) + 2;
    int y = (rand() % window_height - 4) + 4;
    game.enemies[index].coords.x = x;
    game.enemies[index].coords.y = y;
    game.enemies[index].count_move_cooldown = 0;
    game.enemies[index].count_hit_cooldown = 0;

    switch (type){
    case ENEMY_TYPE_1:
        game.enemies[index].type = type;
        game.enemies[index].damage = ENEMY_TYPE_1_DAMAGE;
        game.enemies[index].health = ENEMY_TYPE_1_HEALTH;
        game.enemies[index].alive = 1;
        game.enemies[index].move_cooldown = ENEMY_TYPE_1_MOVE_COOLDOWN;
        game.enemies[index].hit_cooldown = ENEMY_TYPE_1_HIT_COOLDOWN;
        break;
    
    case ENEMY_TYPE_2:
        game.enemies[index].type = type;
        game.enemies[index].damage = ENEMY_TYPE_2_DAMAGE;
        game.enemies[index].health = ENEMY_TYPE_2_HEALTH;
        game.enemies[index].alive = 1;
        game.enemies[index].move_cooldown = ENEMY_TYPE_2_MOVE_COOLDOWN;
        game.enemies[index].hit_cooldown = ENEMY_TYPE_2_HIT_COOLDOWN;
    default:
        break;
    }
}

int enemy_dead(){
    for(int i = 0; i < game.enemy_count; ++i)
        if(game.enemies[i].alive == 0)
            return i;

    return -1;
}

void spawn_enemy(){
    if(enemy_cooldown >= ENEMY_SPAWN_COOLDOWN && game.enemy_count <= MAX_ENEMY_COUNT){
        srand(time(NULL));
        int type = game.stage;
        
        int index = enemy_dead();
        if(index == -1 && game.enemy_count < MAX_ENEMY_COUNT){
            enemy_init(type, game.enemy_count);
            game.enemy_count++;
        }else{
            enemy_init(type, index);
        }

        enemy_cooldown = 0;
    }
}

void move_enemies(){
    for(int i = 0; i < game.enemy_count; ++i){
        if(game.enemies[i].alive){
            if(game.enemies[i].count_move_cooldown >= game.enemies[i].move_cooldown){
                int which_axes = rand() % 2;

                if(which_axes == 1 && game.shooter.coords.x != game.enemies[i].coords.x){
                    if(game.shooter.coords.x > game.enemies[i].coords.x){
                            game.enemies[i].coords.x++;
                    }else{
                        game.enemies[i].coords.x--;
                    }
                }else if(game.shooter.coords.y != game.enemies[i].coords.y){
                    if(game.enemies[i].y_axes_count >= Y_AXES_MOVEMENT_COOLDOWN){
                        if(game.shooter.coords.y > game.enemies[i].coords.y){
                            game.enemies[i].coords.y++;
                        }else{
                            game.enemies[i].coords.y--;
                        }
                        game.enemies[i].y_axes_count = 0;
                    }
                }

                game.enemies[i].count_move_cooldown = 0;
            }
        }
    }
}

//--------------------------------------------------------------------

void bullet_init(int index, int shooting_dir){
    game.shooter.weapon.bullets[index].out_of_bounds = 0;
    game.shooter.weapon.bullets[index].hit = 0;
    game.shooter.weapon.bullets[index].direction = shooting_dir;

    switch (game.stage){
    case 1:
        game.shooter.weapon.bullets[index].hitbox = HITBOX_1;
        break;
    case 2:
        game.shooter.weapon.bullets[index].hitbox = HITBOX_2;
        break;
    default:
        break;
    }
        

    switch (shooting_dir){
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
    case D1:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x + 1;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y - 1;
        if(game.shooter.shooting_dir == UP || game.shooter.shooting_dir == DOWN)
            game.shooter.weapon.bullets[index].main_direction = Y_AXES;
        else
            game.shooter.weapon.bullets[index].main_direction = X_AXES;
        break;
    case D2:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x + 1;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y + 1;
        if(game.shooter.shooting_dir == UP || game.shooter.shooting_dir == DOWN)
            game.shooter.weapon.bullets[index].main_direction = Y_AXES;
        else
            game.shooter.weapon.bullets[index].main_direction = X_AXES;
        break;
        break;
    case D3:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x - 1;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y + 1;
        if(game.shooter.shooting_dir == UP || game.shooter.shooting_dir == DOWN)
            game.shooter.weapon.bullets[index].main_direction = Y_AXES;
        else
            game.shooter.weapon.bullets[index].main_direction = X_AXES;
        break;
        break;
    case D4:
        game.shooter.weapon.bullets[index].coords.x = game.shooter.coords.x - 1;
        game.shooter.weapon.bullets[index].coords.y = game.shooter.coords.y - 1;
        if(game.shooter.shooting_dir == UP || game.shooter.shooting_dir == DOWN)
            game.shooter.weapon.bullets[index].main_direction = Y_AXES;
        else
            game.shooter.weapon.bullets[index].main_direction = X_AXES;
        break;
        break;
    default:
        break;
    }
}

int check_for_available_bullets(){
    for(int i = 0; i < game.bullet_count; ++i)
        if(game.shooter.weapon.bullets[i].out_of_bounds || game.shooter.weapon.bullets[i].hit)
            return i;
        
    return -1;

}


bool check_count_vs_arraysize(){
    return game.bullet_count < MAX_BULLET_COUNT;
}

void shoot_weapon_1(){
    int index = check_for_available_bullets();
    if(index == -1){
        if(check_count_vs_arraysize()){
            bullet_init(game.bullet_count, game.shooter.shooting_dir);
            game.bullet_count++;
        }
    }else{
        bullet_init(index, game.shooter.shooting_dir);
    }
    shoot_cooldown = 0;
}

void shoot_weapon_2(){
    int index = check_for_available_bullets();
    if(index == -1){
        if(check_count_vs_arraysize()){
            bullet_init(game.bullet_count, game.shooter.shooting_dir);
            game.bullet_count++;
        }
    }else{
        bullet_init(index, game.shooter.shooting_dir);
    }


    index = check_for_available_bullets();
    int second_dir = 0;
    int third_dir = 0;

    switch(game.shooter.shooting_dir){
        case UP:
            second_dir = D1;
            third_dir = D4;
            break;
        case RIGHT:
            second_dir = D2;
            third_dir = D1;
            break;
        case DOWN:
            second_dir = D3;
            third_dir = D2;
            break;
        case LEFT:
            second_dir = D4;
            third_dir = D3;
            break;
    }


    if(index == -1){
        if(check_count_vs_arraysize()){
            bullet_init(game.bullet_count, second_dir);
            game.bullet_count++;
        }
    }else{
        bullet_init(index, second_dir);
    }



    index = check_for_available_bullets();
    if(index == -1){
        if(check_count_vs_arraysize()){
            bullet_init(game.bullet_count, third_dir);
            game.bullet_count++;
        }
    }else{
        bullet_init(index, third_dir);
    }



    shoot_cooldown = 0;
}

void shoot(){
    if(shoot_cooldown >= game.shooter.weapon.cooldown){
        switch (game.shooter.weapon.stage){
        case 1:
            shoot_weapon_1();
            break;

        case 2:
            shoot_weapon_2();
            break;
            
        default:
            break;
        }
    }
    
}

void mark_out_of_bounds_bullets(){
    for(int i = 0; i < game.bullet_count; ++i)
        if(game.shooter.weapon.bullets[i].coords.x < 1 || game.shooter.weapon.bullets[i].coords.x > window_width || game.shooter.weapon.bullets[i].coords.y < 1 || game.shooter.weapon.bullets[i].coords.y > window_height)
            game.shooter.weapon.bullets[i].out_of_bounds = 1;
}

void move_bullets(){
    for(int i = 0; i < game.bullet_count; ++i){
        if(!game.shooter.weapon.bullets[i].out_of_bounds && !game.shooter.weapon.bullets[i].hit){
            switch (game.shooter.weapon.bullets[i].direction){
            case UP:
                if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                    game.shooter.weapon.bullets[i].coords.y--;
                    game.shooter.weapon.bullets[i].y_axes_count = 0;
                }
                break;
            case RIGHT:
                game.shooter.weapon.bullets[i].coords.x++;
                break;
            case DOWN:
                if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                    game.shooter.weapon.bullets[i].coords.y++;
                    game.shooter.weapon.bullets[i].y_axes_count = 0;
                }
                break;
            case LEFT:
                game.shooter.weapon.bullets[i].coords.x--;
                break;
            case D1:
                if(game.shooter.weapon.bullets[i].main_direction == Y_AXES){
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y--;
                        game.shooter.weapon.bullets[i].coords.x++;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                }else{
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y--;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                    game.shooter.weapon.bullets[i].coords.x++;
                }
                break;
            case D2:
                if(game.shooter.weapon.bullets[i].main_direction == Y_AXES){
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y++;
                        game.shooter.weapon.bullets[i].coords.x++;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                }else{
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y++;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                    game.shooter.weapon.bullets[i].coords.x++;
                }
                break;
            case D3:
                if(game.shooter.weapon.bullets[i].main_direction == Y_AXES){
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y++;
                        game.shooter.weapon.bullets[i].coords.x--;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                }else{
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y++;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                    game.shooter.weapon.bullets[i].coords.x--;
                }
                break;
            case D4:
                if(game.shooter.weapon.bullets[i].main_direction == Y_AXES){
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y--;
                        game.shooter.weapon.bullets[i].coords.x--;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                }else{
                    if(game.shooter.weapon.bullets[i].y_axes_count >= Y_AXES_BULLETS_COOLDOWN){
                        game.shooter.weapon.bullets[i].coords.y--;

                        game.shooter.weapon.bullets[i].y_axes_count = 0;
                    }
                    game.shooter.weapon.bullets[i].coords.x--;
                }
            default:
                break;
            }
        }
    }
}

//--------------------------------------------------------------------

void check_shooter(){
    if(game.shooter.health <= 0)
        game.is_running = 0;
}

//--------------------------------------------------------------------

void leave_game(){
    game.is_running = 0;
    update_best_score();
    restore_terminal_colour();
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
                    if(game.shooter.y_axes_count >= Y_AXES_MOVEMENT_COOLDOWN){
                        game.shooter.coords.y--;
                        game.shooter.shooting_dir = UP;
                        game.shooter.y_axes_count = 0;
                    }
                    break;
                case 'a':
                    game.shooter.coords.x--;
                    game.shooter.shooting_dir = LEFT;
                    break;
                case 's':
                    if(game.shooter.y_axes_count >= Y_AXES_MOVEMENT_COOLDOWN){
                        game.shooter.coords.y++;
                        game.shooter.shooting_dir = DOWN;
                        game.shooter.y_axes_count = 0;
                    }
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

void listen_for_endscreen_input(){
    char key = getchar();
    if(key == 'r'){
        start_game();
    }else if(key == 'm'){
        start_screen();
    }else if(key == 'q'){
        exit_game();
    }
    listen_for_endscreen_input();
}

//--------------------------------------------------------------------

void stage_up_message(){
    print_at_with_int(window_width/2-12, window_height/2 + 1, "YOU GOT TO THE NEXT STAGE: ", game.stage);
}

void check_stage(){
    if(game.score == STAGE_2_THRESHOLD){
        game.shooter.weapon.stage = 2;
        game.shooter.weapon.cooldown = SHOOT_COOLDOWN_WEAPON_2;
        game.shooter.weapon.damage = WEAPON_2_DAMAGE;
        game.stage = 2;

        stage_up_message();
    }
}

//--------------------------------------------------------------------

void check_red_screen(){
    if(game.red == 1){
        game.red_count++;
        if(game.red_count >= RED_COOLDOWN){
            game.red = 0;
            game.red_count = 0;
            restore_terminal_colour();
        }

    }
}

//--------------------------------------------------------------------

void game_loop(){
    char key = '\0';
    movement = MOVE_COOLDOWN;
    shoot_cooldown = SHOOT_COOLDOWN_WEAPON_1;
    enemy_cooldown = ENEMY_SPAWN_COOLDOWN;
    while(game.is_running){
        fflush(stdout);
        clear_terminal();
        print_elements();

        set_cooldowns();

        move_enemies();
        move_bullets();
        mark_out_of_bounds_bullets();

        check_enemies();
        spawn_enemy();

        check_shooter();

        check_stage();

        check_red_screen();

        listen_for_input(&key);
    } 


    update_best_score();
    clear_terminal();  
    print_death_screen();
    listen_for_endscreen_input();
}

//--------------------------------------------------------------------

void restore_terminal(){
    clear_terminal();
    restore_terminal_colour();
    remove_terminal_indent();
}

void exit_game(){
    restore_terminal();
    printf("\033[?25h");  //show cursor
    exit(1);
}

void start_game(){
    clear_terminal();
    elements_init();
    game_loop();
}


int get_best_score(){
    FILE *f = fopen("score.txt", "r");
    if(f == NULL)
        return 0;

    int score = 0;
    if(fscanf(f, "%d", &score) != 1)
        score = 0;
    
    fclose(f);

    return score;    
}

void update_best_score(){
    if(game.score > get_best_score()){
        FILE *f = fopen("score.txt", "w");

       fprintf(f, "%d", game.score);

        fclose(f);
    }   
}


void start_screen(){
    restore_terminal_colour();
    clear_terminal();
    print_at(window_width/8, window_height/3-3, "Press 's' to start the game");
    print_at(window_width/8, window_height/3-1, "Press 'q' to quit  the game");
    print_at_with_int(window_width/8 + 2, window_height/3+2, "Your best score is: ", get_best_score());

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
