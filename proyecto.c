#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <stdbool.h>
#include <stdio.h>

#define MAXB 3
#define TILE_SIZE 32
#define MAP_ROWS 20 // antes era 15
#define MAP_COL 30 // antes era 20
#define MAX_ENEMIES 10

typedef struct {
    bool W;
    bool A;
    bool S;
    bool D;
    bool up;
    bool down;
    bool left;
    bool right;
} teclado;

typedef struct {
    float x, y;
    float dx, dy;
    bool active;
} bala;

typedef struct {
    float x, y;
    float speed;
    bool disparo;
    float b_dx;
    float b_dy;
    int cooldown;
    int municion;
    int vida;
    int invu_timer;
    bala balas[MAXB];
} jugador;

typedef struct {
    bool objeto;
    bool puerta;
    bool pared;
} tile;

typedef struct {
    tile grid[MAP_ROWS][MAP_COL]; 
} mapa;

typedef struct {
    float x, y;
    bool active;
    int tipo; // 1 = persigue, 2 = Patrulla de arriba a abajo, 3 = dispara
    //    bala balas[MAXB];
    int vida;
    float dy;
} enemigo;

typedef struct {
    jugador player;
    enemigo enemigos[MAX_ENEMIES];
    mapa map;
    teclado keys;    
    bool game_over;
    ALLEGRO_FONT* font; //para guardar la fuente de texto?
} GameState;

bool hay_enemigos_vivos(GameState* state);
bool check_collision(GameState* state, float x, float y);
void load_map(GameState* state, const char* filename);
void init_game(GameState* state);
void manejo_input(GameState* state, ALLEGRO_EVENT* event);
void update_game(GameState* state);
void draw_game(GameState* state);

int main() {
    al_init();
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(960, 640);
    
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    bool redraw = true;
    GameState gs = {0};
    init_game(&gs);

    al_start_timer(timer);

    while (!gs.game_over) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_KEY_DOWN || event.type == ALLEGRO_EVENT_KEY_UP) {
            manejo_input(&gs, &event);
        }
        else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            gs.game_over = true;
        }
    
        if (event.type == ALLEGRO_EVENT_TIMER) {
            update_game(&gs);
            redraw = true;
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            draw_game(&gs);
            redraw = false;
        }
    }

    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_font(gs.font);

    return 0;
}

// funciones

bool hay_enemigos_vivos(GameState* state){
    for (int i = 0; i < MAX_ENEMIES; i++){
        if (state->enemigos[i].active){
            return true; // hay enemigos vivos
        }
    }
    return false; // todos muertos
}

bool check_collision(GameState* state, float x, float y) {
    bool hay_colision = false; 
    int grid_x = (int)x / TILE_SIZE;
    int grid_y = (int)y / TILE_SIZE;
    
    if (grid_x < 0 || grid_x >= MAP_COL || grid_y < 0 || grid_y >= MAP_ROWS) {
        hay_colision = true; 
    } 
    else {
        if (state->map.grid[grid_y][grid_x].pared) {
            hay_colision = true;
        }
        //la puerta solo coliciona si hay enemigos vivos
        else if(state->map.grid[grid_y][grid_x].puerta && hay_enemigos_vivos(state)){
            hay_colision = true;
        }
    }
    
    return hay_colision; 
}

void load_map(GameState* state, const char* filename) {
    FILE* file = fopen(filename, "r");

    if (!file) {
        printf("¡Error! No se pudo abrir el archivo %s. Creando mapa por defecto.\n", filename);
        for (int fil = 0; fil < MAP_ROWS; fil++) {
            for (int col = 0; col < MAP_COL; col++) {
                state->map.grid[fil][col].pared = false;
                state->map.grid[fil][col].puerta = false;
                state->map.grid[fil][col].objeto = false;

                if (fil == 0 || fil == MAP_ROWS - 1 || col == 0 || col == MAP_COL - 1) {
                    state->map.grid[fil][col].pared = true;
                }
            }
        }
        state->player.x = 64;
        state->player.y = 64;
    } 
    else {
        for (int fil = 0; fil < MAP_ROWS; fil++) {
            for (int col = 0; col < MAP_COL; col++) {
                int tipo_entero = 0;
                fscanf(file, "%d", &tipo_entero);

                state->map.grid[fil][col].pared = false;
                state->map.grid[fil][col].puerta = false;
                state->map.grid[fil][col].objeto = false;

                if (tipo_entero == 1) {
                    state->map.grid[fil][col].pared = true;
                } 
                else if (tipo_entero == 2) {
                    state->map.grid[fil][col].puerta = true;
                } 
                else if (tipo_entero == 3) {
                    state->map.grid[fil][col].objeto = true;
                } 
                else if (tipo_entero == 4) {
                    state->player.x = col * TILE_SIZE;
                    state->player.y = fil * TILE_SIZE;
                }
                //Lectura del enemigo
                else if (tipo_entero == 5) {
                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (!state->enemigos[i].active) { // Buscamos un espacio libre en el arreglo
                            state->enemigos[i].x = col * TILE_SIZE;
                            state->enemigos[i].y = fil * TILE_SIZE;
                            state->enemigos[i].active = true;
                            state->enemigos[i].tipo = 1; // 1 = persigue 2 = se mueve de arria a abajo
                            state->enemigos[i].vida = 5;
                            break; 
                        }
                    }
                }
                else if (tipo_entero == 6){
                    for (int i = 0; i < MAX_ENEMIES; i++){
                        if(!state->enemigos[i].active){
                            state->enemigos[i].x = col * TILE_SIZE;
                            state->enemigos[i].y = fil * TILE_SIZE;
                            state->enemigos[i].active = true;
                            state->enemigos[i].tipo = 2;
                            state->enemigos[i].vida = 4;
                            state->enemigos[i].dy = 2.0f; // inicia moviendose hacia abajo a velocidad 2
                            break;
                        }
                    }
                }
            }
        }
        fclose(file);
    }
}

void init_game(GameState* state) {
    state->player.speed = 3.0f;
    state->player.cooldown = 0;
    state->player.disparo = false;
    
    for(int i = 0; i < MAXB; i++) {
        state->player.balas[i].active = false;
    }
    
    state->player.municion = 15;
    state->player.vida = 3; 
    state->player.invu_timer = 0;
    state->keys.W = false;
    state->keys.A = false;
    state->keys.S = false;
    state->keys.D = false;
    state->keys.up = false;
    state->keys.down = false;
    state->keys.left = false;
    state->keys.right = false;
    
    state->game_over = false;

    state->font = al_create_builtin_font();

    load_map(state, "mapa.txt");
}

void manejo_input(GameState* state, ALLEGRO_EVENT* event) {
    if (event->type == ALLEGRO_EVENT_KEY_DOWN) {
        switch (event->keyboard.keycode) {
            case ALLEGRO_KEY_W: 
                state->keys.W = true; 
                break;
            case ALLEGRO_KEY_A: 
                state->keys.A = true; 
                break;
            case ALLEGRO_KEY_S: 
                state->keys.S = true; 
                break;
            case ALLEGRO_KEY_D: 
                state->keys.D = true; 
                break;
            case ALLEGRO_KEY_ESCAPE: 
                state->game_over = true; 
                break; 
            case ALLEGRO_KEY_UP:    
                state->keys.up = true;    
                break;
            case ALLEGRO_KEY_DOWN:  
                state->keys.down = true;  
                break;
            case ALLEGRO_KEY_LEFT:  
                state->keys.left = true;  
                break;
            case ALLEGRO_KEY_RIGHT: 
                state->keys.right = true; 
                break;
        }
    }
    else if (event->type == ALLEGRO_EVENT_KEY_UP) {
        switch (event->keyboard.keycode) {
            case ALLEGRO_KEY_W: 
                state->keys.W = false; 
                break;
            case ALLEGRO_KEY_A: 
                state->keys.A = false; 
                break;
            case ALLEGRO_KEY_S: 
                state->keys.S = false; 
                break;
            case ALLEGRO_KEY_D: 
                state->keys.D = false; 
                break;
            case ALLEGRO_KEY_UP:    
                state->keys.up = false;    
                break;
            case ALLEGRO_KEY_DOWN:  
                state->keys.down = false;  
                break;
            case ALLEGRO_KEY_LEFT:  
                state->keys.left = false;  
                break;
            case ALLEGRO_KEY_RIGHT: 
                state->keys.right = false; 
                break;
        }
    }
}

void update_game(GameState* state) {
    float next_x = state->player.x;
    float next_y = state->player.y;
    float p_size = 30; 
    float shoot_dx = 0;
    float shoot_dy = 0;
    bool intentando_disparar = false;

    if (state->keys.W) {
        next_y -= state->player.speed;
    }
    if (state->keys.A) {
        next_x -= state->player.speed;
    }
    if (state->keys.S) {
        next_y += state->player.speed;
    }
    if (state->keys.D) {
        next_x += state->player.speed;
    }
    //cooldown de las balas
    if (state->player.cooldown > 0) {
        state->player.cooldown--;
    }

    if (state->keys.up) { 
        shoot_dy = -8; 
        intentando_disparar = true; 
    }
    if (state->keys.down) { 
        shoot_dy = 8;  
        intentando_disparar = true; 
    }
    if (state->keys.left) { 
        shoot_dx = -8; 
        intentando_disparar = true; 
    }
    if (state->keys.right) { 
        shoot_dx = 8;  
        intentando_disparar = true; 
    }

    if (intentando_disparar && state->player.cooldown == 0 && state->player.municion > 0) {
        for (int i = 0; i < MAXB; i++) {
            if (!state->player.balas[i].active) {
                state->player.balas[i].x = state->player.x + 15;
                state->player.balas[i].y = state->player.y + 15;
                state->player.balas[i].dx = shoot_dx;
                state->player.balas[i].dy = shoot_dy;
                state->player.balas[i].active = true;
                state->player.cooldown = 15;
                state->player.municion--;
                break;
            }
        }
        state->player.disparo = false;
    }

    if (!check_collision(state, next_x, state->player.y) && !check_collision(state, next_x + p_size, state->player.y) && !check_collision(state, next_x, state->player.y + p_size) && !check_collision(state, next_x + p_size, state->player.y + p_size)) {
        state->player.x = next_x;
    }
    
    if (!check_collision(state, state->player.x, next_y) && !check_collision(state, state->player.x + p_size, next_y) && !check_collision(state, state->player.x, next_y + p_size) && !check_collision(state, state->player.x + p_size, next_y + p_size)) {
        state->player.y = next_y;
    }

    int centro_x = (int)(state->player.x + 15) / TILE_SIZE;
    int centro_y = (int)(state->player.y + 15) / TILE_SIZE;
    
    if (state->map.grid[centro_y][centro_x].puerta) {
        // Lógica de puertas (por hacer)
    }

    if (state->map.grid[centro_y][centro_x].objeto) {
        state->player.municion = 15; // rellena el cargador al máximo
        state->map.grid[centro_y][centro_x].objeto = false; // borra el objeto del mapa
    }

    for (int i = 0; i < MAXB; i++) {
        if (state->player.balas[i].active) {

            state->player.balas[i].x += state->player.balas[i].dx;

            state->player.balas[i].y += state->player.balas[i].dy;

            if (check_collision(state, state->player.balas[i].x, state->player.balas[i].y) || state->player.balas[i].x < 0 || state->player.balas[i].x > 960 || state->player.balas[i].y < 0 || state->player.balas[i].y > 640) {
                
                state->player.balas[i].active = false;
            }
        }
    }
    // IA del enemigo que persigue tipo 1
    for (int i = 0; i < MAX_ENEMIES; i++) {

        if (state->enemigos[i].active && state->enemigos[i].tipo == 1) {
            
            float e_speed = 1.9f; 
            float e_next_x = state->enemigos[i].x;
            float e_next_y = state->enemigos[i].y;
            float e_size = 30.0f;

            if (state->player.x < state->enemigos[i].x){ 
                e_next_x -= e_speed; }
            if (state->player.x > state->enemigos[i].x){ 
                e_next_x += e_speed; }
            if (state->player.y < state->enemigos[i].y){ 
                e_next_y -= e_speed; }
            if (state->player.y > state->enemigos[i].y){ 
                e_next_y += e_speed; }

            if (!check_collision(state, e_next_x, state->enemigos[i].y) && !check_collision(state, e_next_x + e_size, state->enemigos[i].y) && !check_collision(state, e_next_x, state->enemigos[i].y + e_size) && !check_collision(state, e_next_x + e_size, state->enemigos[i].y + e_size)) {
                state->enemigos[i].x = e_next_x;
            }
            
            if (!check_collision(state, state->enemigos[i].x, e_next_y) && !check_collision(state, state->enemigos[i].x + e_size, e_next_y) && !check_collision(state, state->enemigos[i].x, e_next_y + e_size) && !check_collision(state, state->enemigos[i].x + e_size, e_next_y + e_size)) {
                state->enemigos[i].y = e_next_y;
            }
        }

        //tipo 2 
        else if(state->enemigos[i].active && state->enemigos[i].tipo == 2){
            float e_sig_y = state->enemigos[i].y + state->enemigos[i].dy;
            float e_size = 30.0f;

            if (!check_collision(state, state->enemigos[i].x, e_sig_y) && !check_collision(state, state->enemigos[i].x + e_size, e_sig_y) && !check_collision(state, state->enemigos[i].x, e_sig_y + e_size) && !check_collision(state, state->enemigos[i].x + e_size, e_sig_y + e_size)){
               //si no choca se mueve
                state->enemigos[i].y = e_sig_y;
            }
            else {
                // si choca con una pared, cambia de dirección (si bajaba, ahora sube)
                state->enemigos[i].dy = -state->enemigos[i].dy;
            }
        }
    }

    // colision bala contra enemigo
    for (int j = 0; j < MAXB; j++) {
        if (state->player.balas[j].active) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (state->enemigos[i].active) {
                    
                    // Comprobamos si la bala golpea al enemigo
                    if (state->player.balas[j].x >= state->enemigos[i].x && state->player.balas[j].x <= state->enemigos[i].x + 30 && state->player.balas[j].y >= state->enemigos[i].y && state->player.balas[j].y <= state->enemigos[i].y + 30) {
                        
                        //destruimos la bala
                        state->player.balas[j].active = false;
                        
                        //le quitamos 1 de vida al enemigo
                        state->enemigos[i].vida--;
                        
                        //verifica si ya se quedó sin vidas
                        if (state->enemigos[i].vida <= 0) {
                            state->enemigos[i].active = false; //muere el enemigo
                        }
                    }
                }
            }
        }
    }

    //baja el tiempo de la invunerabilidad
    if (state->player.invu_timer > 0){
        state->player.invu_timer--;
    }

    if(state->player.invu_timer == 0){
        for (int i = 0; i < MAX_ENEMIES; i++){
            if (state->enemigos[i].active){

                // Comprobamos colisión entre el jugador (30x30) y el enemigo (30x30)
                if (state->player.x < state->enemigos[i].x + 30 && state->player.x + 30 > state->enemigos[i].x && state->player.y < state->enemigos[i].y + 30 && state->player.y + 30 > state->enemigos[i].y){

                    state->player.vida--; //pierde un corazon
                    state->player.invu_timer = 180; // 3 segundos de invunerabilidad

                    if (state->player.vida <= 0){
                        state->game_over = true;
                    }

                    break;
                }
            }
        }
    }

}

void draw_game(GameState* state) {

    al_clear_to_color(al_map_rgb(20,20,20));

    bool bloquear_puerta = hay_enemigos_vivos(state);

    for (int fil = 0; fil < MAP_ROWS; fil++) {
        for (int col = 0; col < MAP_COL; col++) {
            if (state->map.grid[fil][col].pared) {
                al_draw_filled_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(70, 90, 120));
                al_draw_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(40, 50, 70), 1);
            } 
            else if (state->map.grid[fil][col].puerta) {
                if (bloquear_puerta){
                    //si hay enemigos vivos, la puerta se dibuja en azul
                    al_draw_filled_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(70, 90, 120));
                    al_draw_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(40, 50, 70), 1);
                }
                else {
                al_draw_filled_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(140, 70, 20));
                al_draw_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(90, 40, 10), 1);
                }
            } 
            else if (state->map.grid[fil][col].objeto) {
                al_draw_filled_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(160, 160, 160));
                al_draw_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(100, 100, 100), 1);
            }
        }
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemigos[i].active) {
            if (state->enemigos[i].tipo == 1) {
            al_draw_filled_rectangle(state->enemigos[i].x, state->enemigos[i].y, state->enemigos[i].x + 30, state->enemigos[i].y + 30, al_map_rgb(255, 0, 0));
            }
            if (state->enemigos[i].tipo == 2){
            al_draw_filled_rectangle(state->enemigos[i].x, state->enemigos[i].y, state->enemigos[i].x + 30, state->enemigos[i].y + 30, al_map_rgb(255, 128, 0));
            }
            
        }
    }

    if (state->player.invu_timer == 0 || (state->player.invu_timer / 10) % 2 == 0) {
        al_draw_filled_rectangle(state->player.x, state->player.y, state->player.x + 30, state->player.y + 30, al_map_rgb(0, 255, 0));
    }

    for (int i = 0; i < MAXB; i++) {
        if (state->player.balas[i].active) {
            al_draw_filled_circle(state->player.balas[i].x, state->player.balas[i].y, 4, al_map_rgb(255, 255, 0));
        }
    }

    // Dibuja el texto en la coordenada X=20, Y=450 en color blanco
    al_draw_textf(state->font, al_map_rgb(255, 255, 255), 20, 550, 0, "BALAS: %d / 15           VIDAS: %d", state->player.municion, state->player.vida);
    
    al_flip_display();
}