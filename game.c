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
#define MAX_BULLET_COUNT 200
#define MAX_ENEMY_COUNT 10


#define MOVE_COOLDOWN 4
#define ENEMY_SPAWN_COOLDOWN 300

#define SHOOT_COOLDOWN_WEAPON_1 10
#define WEAPON_1_DAMAGE 10

#define NR_OF_TYPES 1

#define ENEMY_TYPE_1 1
#define ENEMY_TYPE_1_HEALTH 15
#define ENEMY_TYPE_1_DAMAGE 1

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
    bool out_of_bounds;
    bool hit;
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
} Shooter;

typedef struct{
    Coords coords;
    int health;
    int damage;
    int type;
    bool alive;
} Enemy;

typedef struct{
    bool is_running;
    Shooter shooter;
    int kills;
    int best_game;
    int bullet_count;
    Enemy enemies[MAX_ENEMY_COUNT];
    int enemy_count;
} Game;

Game game;

void game_init(){
    game.kills = 0;
    game.is_running = 1;
    game.bullet_count = 0;
    game.enemy_count = 0;
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

void print_elements(){
    print_bullets();
    print_enemies();
    print_shooter();
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

void set_cooldowns(){
    set_movement();
    set_shoot_cooldown();
    set_enemy_spawn_cooldown();
}

//--------------------------------------------------------------------

void check_bullet_collisions(){
    for(int i = 0; i < game.enemy_count; ++i){
        for(int j = 0; j < game.bullet_count; ++j){
            if(game.enemies[i].alive && !game.shooter.weapon.bullets[j].hit && game.enemies[i].coords.x == game.shooter.weapon.bullets[j].coords.x && game.enemies[i].coords.y == game.shooter.weapon.bullets[j].coords.y){
                game.enemies[i].health -= game.shooter.weapon.damage;
                game.shooter.weapon.bullets[j].hit = 1;
            }
        }
    }
}

void check_shooter_collisions(){
    for(int i = 0; i < game.enemy_count; ++i){
        if(game.enemies[i].alive && (game.enemies[i].coords.x == game.shooter.coords.x && game.enemies[i].coords.y == game.shooter.coords.y) || (game.enemies[i].coords.x == game.shooter.coords.x + 1 && game.enemies[i].coords.y == game.shooter.coords.y) || (game.enemies[i].coords.x == game.shooter.coords.x - 1 && game.enemies[i].coords.y == game.shooter.coords.y) || (game.enemies[i].coords.x == game.shooter.coords.x && game.enemies[i].coords.y == game.shooter.coords.y + 1) || (game.enemies[i].coords.x == game.shooter.coords.x && game.enemies[i].coords.y == game.shooter.coords.y - 1)){
            game.shooter.health -= game.enemies[i].damage;
        }
    }
}

void check_dead(){
    for(int i = 0; i < game.enemy_count; ++i)
        if(game.enemies[i].health <= 0)
            game.enemies[i].alive = 0;
}

void check_enemies(){
    check_bullet_collisions();
    check_shooter_collisions();
    check_dead();
}

void enemy_init(int type, int index){
    int x = (rand() % window_width - 2) + 2;
    int y = (rand() % window_height - 4) + 4;
    game.enemies[index].coords.x = x;
    game.enemies[index].coords.y = y;

    switch (type){
    case ENEMY_TYPE_1:
        game.enemies[index].type = type;
        game.enemies[index].damage = ENEMY_TYPE_1_DAMAGE;
        game.enemies[index].health = ENEMY_TYPE_1_HEALTH;
        game.enemies[index].alive = 1;
        break;
    
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
        int type = rand() % NR_OF_TYPES + 1;
        
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

//--------------------------------------------------------------------

void bullet_init(int index){
    game.shooter.weapon.bullets[index].out_of_bounds = 0;
    game.shooter.weapon.bullets[index].hit = 0;
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
            bullet_init(game.bullet_count);
            game.bullet_count++;
        }
    }else{
        bullet_init(index);
    }
    shoot_cooldown = 0;
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
}

//--------------------------------------------------------------------

void check_shooter(){
    if(game.shooter.health <= 0)
        game.is_running = 0;
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
    movement = MOVE_COOLDOWN;
    shoot_cooldown = SHOOT_COOLDOWN_WEAPON_1;
    enemy_cooldown = ENEMY_SPAWN_COOLDOWN;
    while(game.is_running){
        fflush(stdout);
        clear_terminal();
        print_elements();

        set_cooldowns();

        move_bullets();
        mark_out_of_bounds_bullets();

        check_enemies();
        spawn_enemy();

        check_shooter();

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
