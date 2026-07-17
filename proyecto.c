#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_image.h>

#define MAXB 3
#define TILE_SIZE 32
#define MAP_ROWS 20 // antes era 15
#define MAP_COL 30 // antes era 20
#define MAX_ENEMIES 10
#define MAX_EB 20

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
    bool corazon;
} tile;

typedef struct {
    tile grid[MAP_ROWS][MAP_COL]; 
} mapa;

typedef struct {
    // tipo de enemigo
    float x, y;
    bool active;
    int tipo; // 1 = persigue, 2 = Patrulla de arriba a abajo, 3 = dispara
    int estado_ia; //el comportamiento que esta usando el enemigo al momento
    int temp_ia; // cuanto se tarda en cambiar de ia
    int dir_desatoro; // 0 desatoro vertical 1 horizontal
    int Ecooldown;
    int vida;
    float dy;
    float dx;
    bala balasE[MAX_EB];
} enemigo;

typedef struct {
    jugador player;
    enemigo enemigos[MAX_ENEMIES];
    mapa map;
    teclado keys;    
    bool game_over;
    ALLEGRO_FONT* font; //para guardar la fuente de texto
    int nivel;
    ALLEGRO_BITMAP* sprite_pared;
    ALLEGRO_BITMAP* sprite_fondo;
    ALLEGRO_BITMAP* sprite_corazon;
    ALLEGRO_BITMAP* sprite_objeto;
} GameState;

bool hay_enemigos_vivos(GameState* state);
bool check_collision(GameState* state, float x, float y);
bool check_collision_con_enemigos(GameState* state, int mi_indice, float next_x, float next_y);
void load_map(GameState* state, const char* filename);
void init_game(GameState* state);
void manejo_input(GameState* state, ALLEGRO_EVENT* event);
void update_game(GameState* state);
void draw_game(GameState* state);
void cargar_nivel(GameState* state, int nuevo_nivel);

int main() {
    al_init();
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_image_addon();

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
    al_destroy_bitmap(gs.sprite_pared);
    al_destroy_bitmap(gs.sprite_fondo);
    al_destroy_bitmap(gs.sprite_corazon);
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

bool check_collision_con_enemigos(GameState* state, int mi_indice, float next_x, float next_y) {
    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (j != mi_indice && state->enemigos[j].active) {
            
            // comprueba si caja futura (30x30) choca con la caja del enemigo 'j'
            if (next_x < state->enemigos[j].x + 30 && next_x + 30 > state->enemigos[j].x && next_y < state->enemigos[j].y + 30 && next_y + 30 > state->enemigos[j].y) {
                return true;
            }
        }
    }
    return false; 
}

void cargar_nivel(GameState* state, int nuevo_nivel){
    state->nivel = nuevo_nivel;

    
    for(int i = 0; i < MAX_ENEMIES; i++){
        state->enemigos[i].active = false;
    }

    for(int i = 0; i < MAX_ENEMIES; i++){
        for(int b = 0; b < MAX_EB; b++){
            state->enemigos[i].balasE[b].active = false;
        }
    }

    for(int i = 0; i < MAXB ; i++){
        state->player.balas[i].active = false;
    }
    
    if (state->nivel == 1){
        load_map(state, "mapa.txt");
    }
    else if (state->nivel == 2){
        load_map(state, "mapa2.txt");
    }
    else {
        printf("PASASTE TODOS LOS NIVELES!!!!!!\n");
        state->game_over = true;
        return;
    }
    
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
                state->map.grid[fil][col].corazon = false;

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
                            state->enemigos[i].estado_ia = 1; 
                            state->enemigos[i].temp_ia = 0;
                            state->enemigos[i].dir_desatoro = 0;
                            state->enemigos[i].dx = 2.0f;
                            state->enemigos[i].dy = 2.0f;
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
                            state->enemigos[i].estado_ia = 2; 
                            state->enemigos[i].temp_ia = 0;
                            state->enemigos[i].vida = 4;
                            state->enemigos[i].dy = 2.0f; // inicia moviendose hacia abajo a velocidad 2
                            break;
                        }
                    }
                }
                
                else if(tipo_entero == 7){
                    for(int i = 0; i < MAX_ENEMIES; i++){
                        if (!state->enemigos[i].active){
                            state->enemigos[i].x = col * TILE_SIZE;
                            state->enemigos[i].y = fil * TILE_SIZE;
                            state->enemigos[i].active = true;
                            state->enemigos[i].tipo = 3;
                            state->enemigos[i].estado_ia = 3; 
                            state->enemigos[i].temp_ia = 0;
                            state->enemigos[i].vida = 3;
                            state->enemigos[i].Ecooldown = 0;
                            break;
                        }
                    }
                }

                else if (tipo_entero == 8) {
                    state->map.grid[fil][col].corazon = true;
                }

            }
        }
        fclose(file);
    }
}

void init_game(GameState* state) {
    state->sprite_pared = al_load_bitmap("Pared.png");
    state->sprite_fondo = al_load_bitmap("Fondo.png");
    state->sprite_corazon = al_load_bitmap("corazonMAL.png");
    state->sprite_objeto = al_load_bitmap("balas.png");
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

    cargar_nivel(state, 1);
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
        //balas del jugador
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
    
   if (centro_x >= 0 && centro_x < MAP_COL && centro_y >= 0 && centro_y < MAP_ROWS){

        if (state->map.grid[centro_y][centro_x].puerta && !hay_enemigos_vivos(state)){
            cargar_nivel(state, state->nivel + 1);
        }
   }
    //recoje cargador
    if (state->map.grid[centro_y][centro_x].objeto) {
        state->player.municion = 15; // rellena el cargador al máximo
        state->map.grid[centro_y][centro_x].objeto = false; // borra el objeto del mapa
    }
    //recoje corazon
    if (state->map.grid[centro_y][centro_x].corazon) {
        state->player.vida++; // Te suma 1 de vida
        state->map.grid[centro_y][centro_x].corazon = false; // Borra el corazón del mapa
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

    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemigos[i].active) {

            // IA del enemigo que persigue tipo 1
            
            
             switch (state->enemigos[i].estado_ia) {
                
                case 1: { // IA PERSIGUE
                    float e_speed = 1.9f; 
                    float e_next_x = state->enemigos[i].x;
                    float e_next_y = state->enemigos[i].y;
                    float e_size = 30.0f;
                    
                    // Margen de error para pasillos estrechos
                    float mar = 2.0f; 

                    bool choco_x = false;
                    bool choco_y = false;

                    if (state->player.x < state->enemigos[i].x){ 
                        e_next_x -= e_speed;} 
                    if (state->player.x > state->enemigos[i].x){
                         e_next_x += e_speed;} 
                    if (state->player.y < state->enemigos[i].y){
                         e_next_y -= e_speed;} 
                    if (state->player.y > state->enemigos[i].y){
                         e_next_y += e_speed;} 

                    // Movimiento en X
                    if (e_next_x != state->enemigos[i].x) {
                        if (!check_collision(state, e_next_x + mar, state->enemigos[i].y + mar) && !check_collision(state, e_next_x + e_size - mar, state->enemigos[i].y + mar) && !check_collision(state, e_next_x + mar, state->enemigos[i].y + e_size - mar) && !check_collision(state, e_next_x + e_size - mar, state->enemigos[i].y + e_size - mar)) {
                            if (!check_collision_con_enemigos(state, i, e_next_x, state->enemigos[i].y)){
                                state->enemigos[i].x = e_next_x;
                            } else choco_x = true;
                        } else choco_x = true;
                    }

                    // Movimiento en Y
                    if (e_next_y != state->enemigos[i].y) {
                        if (!check_collision(state, state->enemigos[i].x + mar, e_next_y + mar) && !check_collision(state, state->enemigos[i].x + e_size - mar, e_next_y + mar) && !check_collision(state, state->enemigos[i].x + mar, e_next_y + e_size - mar) && !check_collision(state, state->enemigos[i].x + e_size - mar, e_next_y + e_size - mar)) {            
                            if (!check_collision_con_enemigos(state, i, state->enemigos[i].x, e_next_y)){
                                state->enemigos[i].y = e_next_y;
                            } else choco_y = true;
                        } else choco_y = true;
                    }

                    // --- CAMBIO DE IA ---
                    if (state->enemigos[i].tipo == 1 && (choco_x || choco_y)) {
                        state->enemigos[i].estado_ia = 2; 
                        state->enemigos[i].temp_ia = 45;  

                        if (choco_y) {
                            // Si chocó arriba/abajo, se desatora moviéndose de izquierda a derecha
                            state->enemigos[i].dir_desatoro = 1; 
                        } else {
                            // Si chocó en los lados, se desatora moviéndose de arriba a abajo
                            state->enemigos[i].dir_desatoro = 0;
                        }
                    }
                    if (state->enemigos[i].tipo == 3) {
                        float dif_x = state->player.x - state->enemigos[i].x;
                        float dif_y = state->player.y - state->enemigos[i].y;
                        float distancia = sqrt(dif_x * dif_x + dif_y * dif_y);

                        // Si ya se acercó a 200 píxeles o chocó con una pared, se detiene a disparar
                        if (distancia <= 120.0f || choco_x || choco_y) {
                            state->enemigos[i].estado_ia = 3;
                        }
                    }
                    break;
                }
        

                //tipo 2 
               case 2: { // IA DESATORO / PATRULLA
                    float e_size = 30.0f;
                    float mar = 2.0f; 

                    // desatoto Horizontal (isquierda / derecha)
                    if (state->enemigos[i].tipo == 1 && state->enemigos[i].dir_desatoro == 1) {
                        float e_sig_x = state->enemigos[i].x + state->enemigos[i].dx;

                        if (!check_collision(state, e_sig_x + mar, state->enemigos[i].y + mar) && !check_collision(state, e_sig_x + e_size - mar, state->enemigos[i].y + mar) && !check_collision(state, e_sig_x + mar, state->enemigos[i].y + e_size - mar) && !check_collision(state, e_sig_x + e_size - mar, state->enemigos[i].y + e_size - mar) && !check_collision_con_enemigos(state, i, e_sig_x, state->enemigos[i].y)){
                            state->enemigos[i].x = e_sig_x;
                        } else {
                            state->enemigos[i].dx = -state->enemigos[i].dx; // rebota horizontalmente
                        }
                    } 
                    // desatoro vertical (Arriba / Abajo) tambien sirve para patrullaje normal del tipo 2
                    else {
                        float e_sig_y = state->enemigos[i].y + state->enemigos[i].dy;

                        if (!check_collision(state, state->enemigos[i].x + mar, e_sig_y + mar) && !check_collision(state, state->enemigos[i].x + e_size - mar, e_sig_y + mar) && !check_collision(state, state->enemigos[i].x + mar, e_sig_y + e_size - mar) && !check_collision(state, state->enemigos[i].x + e_size - mar, e_sig_y + e_size - mar) && !check_collision_con_enemigos(state, i, state->enemigos[i].x, e_sig_y)){
                            state->enemigos[i].y = e_sig_y;
                        } else {
                            state->enemigos[i].dy = -state->enemigos[i].dy; // Rebota verticalmente
                        }
                    }

                    // REGRESO A IA PERSIGUE
                    if (state->enemigos[i].tipo == 1) {
                        state->enemigos[i].temp_ia--; 
                        if (state->enemigos[i].temp_ia <= 0) {
                            state->enemigos[i].estado_ia = 1; 
                        }
                    }
                    break;
                }


                    // Enemigo tipo 3
                    case 3: { // IA DISPARA
                    //cooldown de balas
                    if (state->enemigos[i].Ecooldown > 0){
                        state->enemigos[i].Ecooldown--;
                    }
                    else {
                        state->enemigos[i].Ecooldown = 85;
                        for(int b = 0; b < MAX_EB; b++){
                            if (!state->enemigos[i].balasE[b].active){
                                state->enemigos[i].balasE[b].active = true;

                                state->enemigos[i].balasE[b].x = state->enemigos[i].x + 15;
                                state->enemigos[i].balasE[b].y = state->enemigos[i].y + 15;

                                //calcula direccion del jugador
                                float dif_x = state->player.x - state->enemigos[i].x;
                                float dif_y = state->player.y - state->enemigos[i].y;
                                float distancia = sqrt(dif_x * dif_x + dif_y * dif_y);

                                //si la distancia es muy corta, evita que divida por 0
                                if (distancia == 0){
                                    distancia = 1;
                                }
                        
                                // normalizamos la dirección y multiplicamos por la velocidad de la bala
                                float vel_bala = 0.5f;
                                state->enemigos[i].balasE[b].dx = (dif_x / distancia) * vel_bala;
                                state->enemigos[i].balasE[b].dy = (dif_y / distancia) * vel_bala;
                                break;
                            }
                        }
                    }

                    float dif_x = state->player.x - state->enemigos[i].x;
                    float dif_y = state->player.y - state->enemigos[i].y;
                    float distancia = sqrt(dif_x * dif_x + dif_y * dif_y);

                    // Si el jugador se aleja a más de 320 píxeles, el enemigo lo vuelve a perseguir
                    if (distancia > 300.0f) {
                        state->enemigos[i].estado_ia = 1;
                    }
                    break;
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

for(int i = 0; i < MAX_ENEMIES; i++){
    for (int b = 0; b < MAX_EB; b++) {
        if (state->enemigos[i].balasE[b].active){
            // mueve la bala
            state->enemigos[i].balasE[b].x += state->enemigos[i].balasE[b].dx;
            state->enemigos[i].balasE[b].y += state->enemigos[i].balasE[b].dy;

            //colision con paredes o fuera de pantalla
            if (check_collision(state, state->enemigos[i].balasE[b].x, state->enemigos[i].balasE[b].y) || state->enemigos[i].balasE[b].x < 0 || state->enemigos[i].balasE[b].x > 960 || state->enemigos[i].balasE[b].y < 0 || state->enemigos[i].balasE[b].y > 640) {
                state->enemigos[i].balasE[b].active = false;
            }
                //colision contra jugador si no es invulnerable
            else if (state->player.invu_timer == 0){
                if (state->enemigos[i].balasE[b].x >= state->player.x && state->enemigos[i].balasE[b].x <= state->player.x + 30 && state->enemigos[i].balasE[b].y >= state->player.y && state->enemigos[i].balasE[b].y <= state->player.y + 30) {
                    
                    state->enemigos[i].balasE[b].active = false;
                    state->player.vida--;
                    state->player.invu_timer = 300; // 3 segundos de invunerabilidad

                    if (state->player.vida <= 0){
                        state->game_over = true;
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
                        state->player.invu_timer = 300; // 3 segundos de invunerabilidad

                        if (state->player.vida <= 0){
                            state->game_over = true;
                        }

                    break;
                    }
                }
            }
        }
    }
}

void draw_game(GameState* state) {

    al_clear_to_color(al_map_rgb(20,20,20));

   if (state->sprite_fondo) {
        float ancho_original = al_get_bitmap_width(state->sprite_fondo);
        float alto_original = al_get_bitmap_height(state->sprite_fondo);

        al_draw_scaled_bitmap(state->sprite_fondo,0, 0, ancho_original, alto_original, 0, 0, 960, 640, 0);
    }

    bool bloquear_puerta = hay_enemigos_vivos(state);

    for (int fil = 0; fil < MAP_ROWS; fil++) {
        for (int col = 0; col < MAP_COL; col++) {
            if (state->map.grid[fil][col].pared) {
                al_draw_bitmap(state->sprite_pared, col * TILE_SIZE, fil * TILE_SIZE, 0);
            } 
            else if (state->map.grid[fil][col].puerta) {
                if (bloquear_puerta){
                    //si hay enemigos vivos, la puerta se dibuja en azul
                    al_draw_bitmap(state->sprite_pared, col * TILE_SIZE, fil * TILE_SIZE, 0);
                }
                else {
                al_draw_filled_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(140, 70, 20));
                al_draw_rectangle(col * TILE_SIZE, fil * TILE_SIZE, (col + 1) * TILE_SIZE, (fil + 1) * TILE_SIZE, al_map_rgb(90, 40, 10), 1);
                }
            } 
            else if (state->map.grid[fil][col].objeto) {
               al_draw_bitmap(state->sprite_objeto, col * TILE_SIZE, fil * TILE_SIZE, 0);
            }
            else if (state->map.grid[fil][col].corazon) {
                if (state->sprite_corazon) {
                    al_draw_bitmap(state->sprite_corazon, col * TILE_SIZE, fil * TILE_SIZE, 0);
                }
            }
        }
    }
    
    //dibuja los tipos de enemigos
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemigos[i].active) {
            if (state->enemigos[i].tipo == 1) {
            al_draw_filled_rectangle(state->enemigos[i].x, state->enemigos[i].y, state->enemigos[i].x + 30, state->enemigos[i].y + 30, al_map_rgb(255, 0, 0));
            }
            if (state->enemigos[i].tipo == 2){
            al_draw_filled_rectangle(state->enemigos[i].x, state->enemigos[i].y, state->enemigos[i].x + 30, state->enemigos[i].y + 30, al_map_rgb(255, 128, 0));
            }
            if (state->enemigos[i].tipo == 3){
            al_draw_filled_rectangle(state->enemigos[i].x, state->enemigos[i].y, state->enemigos[i].x + 30, state->enemigos[i].y + 30, al_map_rgb(148, 0, 211));
            }
            
        }
    }

    if (state->player.invu_timer == 0 || (state->player.invu_timer / 25) % 2 == 0) {
        al_draw_filled_rectangle(state->player.x, state->player.y, state->player.x + 30, state->player.y + 30, al_map_rgb(0, 255, 0));
    }

    for (int i = 0; i < MAXB; i++) {
        if (state->player.balas[i].active) {
            al_draw_filled_circle(state->player.balas[i].x, state->player.balas[i].y, 4, al_map_rgb(255, 255, 0));
        }
    }
    for (int i = 0; i < MAX_ENEMIES; i++){
        for (int b = 0; b < MAX_EB; b++) {
            if (state->enemigos[i].balasE[b].active) {
                al_draw_filled_circle(state->enemigos[i].balasE[b].x, state->enemigos[i].balasE[b].y, 3, al_map_rgb(255, 50, 50));
            }
        }
    }
    // Dibuja el texto en la coordenada X=20, Y=450 en color blanco
    al_draw_textf(state->font, al_map_rgb(255, 255, 255), 20, 550, 0, "BALAS: %d / 15         VIDAS: %d", state->player.municion, state->player.vida);
    
    al_flip_display();
}