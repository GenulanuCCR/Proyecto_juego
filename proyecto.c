#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_image.h>
#include <stdlib.h>
#include <time.h>

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
    float distancia_recorrida;
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
    float rango_max;
    int monedas;
    int daño_ataque;
    bool tiene_defensa;
    bool turno_defensa;
    //son para la animacion del jugador
    int frame_actual;
    int timer_animacion; // un contador para saber cuaqndo cambiar de frame
    int estado_animacion; //0 quieto, 1 moviendose
    int direccion;
    bala balas[MAXB];
} jugador;

typedef struct {
    bool objeto;
    bool puerta;
    bool pared;
    bool corazon;
    bool moneda;
    bool cumulo_monedas;
    int estado_cofre; // 0 nada, 1 oculto, 2 cerrado, 3 abierto
    int objeto_tienda;
    int precio_tienda; 
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
    float size;
    bala balasE[MAX_EB];
} enemigo;

typedef struct {
    jugador player;
    enemigo enemigos[MAX_ENEMIES];
    mapa map;
    teclado keys;    
    bool game_over;
    int nivel;
    int nivel_actual;
    bool en_tienda;
    const char* mensaje_tienda;
    int estadoapp; // 0 menu 1 jugando 2 pantalla de muerte
    ALLEGRO_FONT* font; //para guardar la fuente de texto
    ALLEGRO_BITMAP* sprite_pared;
    ALLEGRO_BITMAP* sprite_fondo;
    ALLEGRO_BITMAP* sprite_corazon;
    ALLEGRO_BITMAP* sprite_objeto;
    ALLEGRO_BITMAP* sprite_ene1;
    ALLEGRO_BITMAP* sprite_ene2;
    ALLEGRO_BITMAP* sprite_ene3;
    ALLEGRO_BITMAP* sprite_moneda;
    ALLEGRO_BITMAP* sprite_cumulo_monedas;
    ALLEGRO_BITMAP* sprite_cofre_cerrado;
    ALLEGRO_BITMAP* sprite_cofre_abierto;
    ALLEGRO_BITMAP* sprite_bebida_daño;
    ALLEGRO_BITMAP* sprite_bebida_rango;
    ALLEGRO_BITMAP* sprite_bebida_vel;
    ALLEGRO_BITMAP* sprite_bebida_def;
    ALLEGRO_BITMAP* sprite_jugador_idle;
    ALLEGRO_BITMAP* sprite_jugador_run;

} GameState;

bool hay_enemigos_vivos(GameState* state);
bool check_collision(GameState* state, float x, float y);
bool check_collision_con_enemigos(GameState* state, int mi_indice, float next_x, float next_y);
void load_map(GameState* state, const char* filename);
void init_game(GameState* state);
void manejo_input(GameState* state, ALLEGRO_EVENT* event);
void update_game(GameState* state);
void draw_game(GameState* state);
void cargar_nivel(GameState* state);

int main() {
    al_init();
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_image_addon();
    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);

    ALLEGRO_DISPLAY* disp = al_create_display(960, 640);
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    
    float monitor_w = al_get_display_width(disp);
    float monitor_h = al_get_display_height(disp);

    float escala_x = monitor_w / 960.0f;
    float escala_y = monitor_h / 640.0f;

    ALLEGRO_TRANSFORM transform;
    al_identity_transform(&transform);
    al_scale_transform(&transform, escala_x, escala_y);
    al_use_transform(&transform);
    
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
    al_destroy_bitmap(gs.sprite_objeto);
    al_destroy_bitmap(gs.sprite_ene1);
    al_destroy_bitmap(gs.sprite_ene2);
    al_destroy_bitmap(gs.sprite_ene3);
    al_destroy_bitmap(gs.sprite_moneda);
    al_destroy_bitmap(gs.sprite_cofre_cerrado);
    al_destroy_bitmap(gs.sprite_cofre_abierto);
    al_destroy_bitmap(gs.sprite_cumulo_monedas);
    al_destroy_bitmap(gs.sprite_bebida_daño);
    al_destroy_bitmap(gs.sprite_bebida_def);
    al_destroy_bitmap(gs.sprite_bebida_vel);
    al_destroy_bitmap(gs.sprite_bebida_rango);
    al_destroy_bitmap(gs.sprite_jugador_idle);
    al_destroy_bitmap(gs.sprite_jugador_run);
    
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
    bool hay_colision = false;

    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (j != mi_indice && state->enemigos[j].active) {
            
            // Extraemos los tamaños reales de ambos enemigos para no usar el 30 fijo
            float mi_size = state->enemigos[mi_indice].size;
            float otro_size = state->enemigos[j].size;
            
            // comprueba si la caja futura choca con la caja real del enemigo 'j'
          if (next_x < state->enemigos[j].x + otro_size && 
                next_x + mi_size > state->enemigos[j].x && 
                next_y < state->enemigos[j].y + otro_size && 
                next_y + mi_size > state->enemigos[j].y) {
                
                hay_colision = true; // Registramos el impacto
                break; // Rompemos el ciclo para optimizar recursos
            }
        }
    }
    return hay_colision; // Único punto de salida
}

void cargar_nivel(GameState* state) {
    char nombre_archivo[50];

    // CASO A: Venimos de salir de la Tienda
    if (state->en_tienda) {
        state->en_tienda = false;
        // Cargamos el nivel principal que nos correspondía
        sprintf(nombre_archivo, "mapa%d.txt", state->nivel);
    } 
    // CASO B: Venimos de completar un Nivel Normal
    else {
        // Avanzamos el contador a tu siguiente nivel
        state->nivel++;

        // Sorteo: 50% de probabilidad de que aparezca la tienda (ajustable)
        int probabilidad_tienda = rand() % 100;

        if (probabilidad_tienda < 75) { 
            state->en_tienda = true;
            sprintf(nombre_archivo, "tienda.txt");
        } else {
            state->en_tienda = false;
            sprintf(nombre_archivo, "mapa%d.txt", state->nivel);
        }
    }

    // Cargamos el mapa correspondiente
    load_map(state, nombre_archivo);
}

void load_map(GameState* state, const char* filename) {
    FILE* file = fopen(filename, "r");

    // Limpieza de enemigos y proyectiles
    for (int i = 0; i < MAX_ENEMIES; i++) {
        state->enemigos[i].active = false;
        for (int b = 0; b < MAX_EB; b++) {
            state->enemigos[i].balasE[b].active = false;
        }
    }
    for (int i = 0; i < MAXB; i++) {
        state->player.balas[i].active = false;
    }

    if (!file) {
        printf("¡Error! No se pudo abrir el archivo %s. Creando mapa por defecto.\n", filename);
        for (int fil = 0; fil < MAP_ROWS; fil++) {
            for (int col = 0; col < MAP_COL; col++) {
                state->map.grid[fil][col].pared = false;
                state->map.grid[fil][col].puerta = false;
                state->map.grid[fil][col].objeto = false;
                state->map.grid[fil][col].corazon = false;
                state->map.grid[fil][col].moneda = false;
                state->map.grid[fil][col].estado_cofre = 0;
                state->map.grid[fil][col].cumulo_monedas = false;
                state->map.grid[fil][col].objeto_tienda = 0;
                state->map.grid[fil][col].precio_tienda = 0;

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
                if (fscanf(file, "%d", &tipo_entero) != 1) {
                    tipo_entero = 0; // Evita basura si el archivo tiene menos datos de los esperados
                }

                state->map.grid[fil][col].pared = false;
                state->map.grid[fil][col].puerta = false;
                state->map.grid[fil][col].objeto = false;
                state->map.grid[fil][col].corazon = false;
                state->map.grid[fil][col].moneda = false;
                state->map.grid[fil][col].estado_cofre = 0;
                state->map.grid[fil][col].cumulo_monedas = false;
                state->map.grid[fil][col].objeto_tienda = 0;
                state->map.grid[fil][col].precio_tienda = 0;

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
                else if (tipo_entero == 5) {
                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (!state->enemigos[i].active) {
                            state->enemigos[i].x = col * TILE_SIZE;
                            state->enemigos[i].y = fil * TILE_SIZE;
                            state->enemigos[i].active = true;
                            state->enemigos[i].tipo = 1;
                            state->enemigos[i].estado_ia = 1; 
                            state->enemigos[i].temp_ia = 0;
                            state->enemigos[i].dir_desatoro = 0;
                            state->enemigos[i].dx = 2.0f;
                            state->enemigos[i].dy = 2.0f;
                            state->enemigos[i].vida = 5;
                            state->enemigos[i].size = 30.0f;
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
                            state->enemigos[i].size = 30.0f;
                            state->enemigos[i].dy = 2.0f;
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
                            state->enemigos[i].size = 30.0f;
                            state->enemigos[i].Ecooldown = 0;
                            break;
                        }
                    }
                }
                else if(tipo_entero == 9){
                    for(int i = 0; i < MAX_ENEMIES; i++){
                        if (!state->enemigos[i].active){
                            state->enemigos[i].x = col * TILE_SIZE;
                            state->enemigos[i].y = fil * TILE_SIZE;
                            state->enemigos[i].active = true;
                            state->enemigos[i].tipo = 4;
                            state->enemigos[i].estado_ia = 4;
                            state->enemigos[i].size = 60.0f;
                            state->enemigos[i].vida = 30;
                            state->enemigos[i].Ecooldown = 100;
                            break;
                        }
                    }
                }
                else if (tipo_entero == 8) {
                    state->map.grid[fil][col].corazon = true;
                }
                else if (tipo_entero == 10) {
                    state->map.grid[fil][col].estado_cofre = 1;
                }
                else if (tipo_entero == 11) {
                    // Si ya tiene defensa, solo puede salir del objeto 1 al 5
                    int max_objetos;
                    if (state->player.tiene_defensa){
                        max_objetos = 5;
                    }
                    else{
                        max_objetos = 6;
                    }
                    state->map.grid[fil][col].objeto_tienda = (rand() % max_objetos) + 1;

                    if (state->map.grid[fil][col].objeto_tienda <= 2) {
                    state->map.grid[fil][col].precio_tienda = 3; 
                    } 
                    else {
                      state->map.grid[fil][col].precio_tienda = 10;
                    }
                }
            }
        }
        fclose(file);
    }
}

void reiniciar_partida(GameState* state) {
    state->nivel = 1;
    state->nivel_actual = 1;
    state->en_tienda = false;
    state->player.speed = 3.0f;
    state->player.daño_ataque = 1;
    state->player.tiene_defensa = false;
    state->player.turno_defensa = false;
    state->player.cooldown = 0;
    state->player.disparo = false;
    state->player.rango_max = 250.0f;
    state->player.monedas = 0;
    state->player.frame_actual = 0;
    state->player.timer_animacion = 0;
    state->player.estado_animacion = 0;
    state->player.direccion = 0;
    state->player.municion = 15;
    state->player.vida = 3;
    state->player.invu_timer = 0;
    
    // Apagar las balas
    for(int i = 0; i < MAXB; i++) {
        state->player.balas[i].active = false;
    }

    // Soltar las teclas
    state->keys.W = false; state->keys.A = false;
    state->keys.S = false; state->keys.D = false;
    state->keys.up = false; state->keys.down = false;
    state->keys.left = false; state->keys.right = false;

    // Volver a cargar el nivel 1
    load_map(state, "mapa1.txt");
}

void init_game(GameState* state) {
    
    srand(time(NULL));
    state->nivel = 1;
    state->nivel_actual = 1;
    state->font = al_create_builtin_font();

    state->sprite_pared = al_load_bitmap("Pared.png");
    state->sprite_fondo = al_load_bitmap("Fondo.png");
    state->sprite_corazon = al_load_bitmap("corazonMAL.png");
    state->sprite_objeto = al_load_bitmap("balas.png");
    state->sprite_ene1 = al_load_bitmap("eneT1.png");
    state->sprite_ene2 = al_load_bitmap("eneT2.png");
    state->sprite_ene3 = al_load_bitmap("eneT3.png");
    state->sprite_moneda = al_load_bitmap("Moneda.png");
    state->sprite_cofre_cerrado = al_load_bitmap("CofreC.png");
    state->sprite_cofre_abierto = al_load_bitmap("CofreA.png");
    state->sprite_cumulo_monedas = al_load_bitmap("Monedas.png");
    state->sprite_bebida_daño = al_load_bitmap("bebida_daño.png");
    state->sprite_bebida_rango = al_load_bitmap("bebida_rango.png");
    state->sprite_bebida_vel = al_load_bitmap("bebida_vel.png");
    state->sprite_bebida_def = al_load_bitmap("bebida_def.png");
    state->sprite_jugador_idle = al_load_bitmap("quieto.png");
    state->sprite_jugador_run = al_load_bitmap("corriendo.png");
    
    state->nivel_actual = 1;
    state->en_tienda = false;
    state->player.speed = 3.0f;
    state->player.daño_ataque = 1;      // Las balas quitan 1 de vida normalmente
    state->player.tiene_defensa = false;
    state->player.turno_defensa = false;
    state->player.cooldown = 0;
    state->player.disparo = false;
    state->player.rango_max = 250.0f;
    state->player.monedas = 0;
    state->player.frame_actual = 0;
    state->player.timer_animacion = 0;
    state->player.estado_animacion = 0;
    state->player.direccion = 0;

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
    
    state->nivel = 1;
    state->game_over = false;
    state->estadoapp = 0;

    state->font = al_create_builtin_font();

    load_map(state, "mapa1.txt");
}

void manejo_input(GameState* state, ALLEGRO_EVENT* event) {
    if (event->type == ALLEGRO_EVENT_KEY_DOWN) {

        if(state->estadoapp == 1){

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
        if(event->keyboard.keycode == ALLEGRO_KEY_ENTER){
            if(state->estadoapp == 0 || state->estadoapp == 2){
                state->estadoapp = 1;//cambiamos a jugando
                reiniciar_partida(state);
            }
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
    bool en_movimiento = false;

    if(state->estadoapp != 1){
        return;
    }

    if (state->keys.W) {
        next_y -= state->player.speed;
        state->player.direccion = 1; //arriba
        en_movimiento = true;
    }
    if (state->keys.A) {
        next_x -= state->player.speed;
         state->player.direccion = 2;//izquierda
        en_movimiento = true;
    }
    if (state->keys.S) {
        next_y += state->player.speed;
         state->player.direccion = 0;//abajo
        en_movimiento = true;
    }
    if (state->keys.D) {
        next_x += state->player.speed;
         state->player.direccion = 3;//derecha
        en_movimiento = true;
    }

    int estado_anterior = state->player.estado_animacion;

    if (en_movimiento) {
        state->player.estado_animacion = 1;//esta corriendo
    }
    else {
        state->player.estado_animacion = 0; //quieto o idle
    }
    if(estado_anterior != state->player.estado_animacion) {
        state->player.frame_actual = 0;
        state->player.timer_animacion = 0;
    }

    int max_frames;
    int tiempo_por_frame;
    
    if(state->player.estado_animacion == 1){
        //config para correr
        max_frames = 10;
        tiempo_por_frame = 5;
    } else {
        max_frames = 8;
        tiempo_por_frame = 9;
    }

    state->player.timer_animacion++;
    if (state->player.timer_animacion >= tiempo_por_frame){
        state->player.timer_animacion = 0;
        state->player.frame_actual++;

        if(state->player.frame_actual >= max_frames){
        state->player.frame_actual = 0;
        }
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
                state->player.balas[i].distancia_recorrida = 0.0f;
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

    if (!hay_enemigos_vivos(state)) {
        for (int f = 0; f < MAP_ROWS; f++) {
            for (int c = 0; c < MAP_COL; c++) {
                // Si estaba oculto (1), lo volvemos cerrado y visible (2)
                if (state->map.grid[f][c].estado_cofre == 1) {
                    state->map.grid[f][c].estado_cofre = 2; 
                }
            }
        }
    }
    
   if (centro_x >= 0 && centro_x < MAP_COL && centro_y >= 0 && centro_y < MAP_ROWS){
     // revisa si murieron todos los enemigos y si esta tocando la puerta
        if (state->map.grid[centro_y][centro_x].puerta && !hay_enemigos_vivos(state)){
            cargar_nivel(state);
        }
   }
    //recoje cargador
    if (state->map.grid[centro_y][centro_x].objeto) {
        state->player.municion = 15; // rellena el cargador al máximo
        state->map.grid[centro_y][centro_x].objeto = false;
    }
    //recoje corazon
    if (state->map.grid[centro_y][centro_x].corazon) {
        state->player.vida++; // Te suma 1 de vida
        state->map.grid[centro_y][centro_x].corazon = false;
    }
    //recoje moneda
    if (state->map.grid[centro_y][centro_x].moneda) {
        state->player.monedas++; // suma 1 a tu variable de jugador
        state->map.grid[centro_y][centro_x].moneda = false;
    }
    //recoje cumulo de monedas
    if (state->map.grid[centro_y][centro_x].cumulo_monedas) {
        state->player.monedas += 4; // te da 4 monedas de golpe
        state->map.grid[centro_y][centro_x].cumulo_monedas = false;
    }
    if (state->map.grid[centro_y][centro_x].estado_cofre == 2) {
        state->map.grid[centro_y][centro_x].estado_cofre = 3;
        // matriz de coordenadas para revisar arriba, abajo, isquierda y derecha
        int direcciones_brujula[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        
        // esta guarda que casillas estan vacias vacías
        int lista_cas_libres[4][2]; 
        int total_cas_libres = 0; 
        
        // escanea el área alrededor del cofre
        for(int direccion = 0; direccion < 4; direccion++) {
            
            // calcula cual es la casilla que vamos a mirar
            int casilla_revisar_x = centro_x + direcciones_brujula[direccion][0];
            int casilla_revisar_y = centro_y + direcciones_brujula[direccion][1];
            
            // verifica que la casilla no este fuera de los límites de la pantalla
            if (casilla_revisar_x >= 0 && casilla_revisar_x < MAP_COL && casilla_revisar_y >= 0 && casilla_revisar_y < MAP_ROWS) {
                
                // verifica que la casilla este vacia (sin pared, puerta o cofre)
                if (!state->map.grid[casilla_revisar_y][casilla_revisar_x].pared && !state->map.grid[casilla_revisar_y][casilla_revisar_x].puerta && state->map.grid[casilla_revisar_y][casilla_revisar_x].estado_cofre == 0) {
                    
                    // si está limpia, anotamos sus coordenadas 'X' e 'Y' en nuestra libreta
                    lista_cas_libres[total_cas_libres][0] = casilla_revisar_x;
                    lista_cas_libres[total_cas_libres][1] = casilla_revisar_y;
                    total_cas_libres++; // suma 1 al contador de casillas libres
                }
            }
        }
        
        // decide cuantos premios soltar. Si encontramos 2 o más casillas libres, soltamos 2
        // Si encuentra solo 1 casilla libre, suelta solo 1 premio
        int premios_a_soltar = (total_cas_libres < 2) ? total_cas_libres : 2; 
        
        //  el sorteo para ver en qué casillas libres caeran los premios
        for (int i = 0; i < premios_a_soltar; i++) {
            
            // Escogemos un número de renglón al azar de nuestra libreta
            int numero_sorteo = rand() % total_cas_libres; 
            
            // lee las coordenadas 'X' e 'Y' de la casilla ganadora
            int casilla_elegida_x = lista_cas_libres[numero_sorteo][0];
            int casilla_elegida_y = lista_cas_libres[numero_sorteo][1];
            
            // hacemos aparecer el botín en esa casilla elegida
            state->map.grid[casilla_elegida_y][casilla_elegida_x].cumulo_monedas = true;
            
            // tachamos esta casilla de la libreta para no volver a elegirla en la siguiente vuelta.
            // lo hacemos sobreescribiéndola con la última casilla de la lista.
            lista_cas_libres[numero_sorteo][0] = lista_cas_libres[total_cas_libres - 1][0];
            lista_cas_libres[numero_sorteo][1] = lista_cas_libres[total_cas_libres - 1][1];
            
            // restamos 1 al total porque ahora tenemos una casilla libre menos disponible
            total_cas_libres--; 
        }
    }

    //comprar en la tienda
    int obj_actual = state->map.grid[centro_y][centro_x].objeto_tienda;
    int precio_actual = state->map.grid[centro_y][centro_x].precio_tienda;

    if (obj_actual > 0) {
        if (state->player.monedas >= precio_actual) {
            
            // Pagamos el objeto
            state->player.monedas -= precio_actual;
            
            // aplicamos el efecto según lo que compramos
            if (obj_actual == 1){
                 state->player.vida++;
            }
            else if (obj_actual == 2){ 
                state->player.municion += 10;
            }
            else if (obj_actual == 3){
                state->player.daño_ataque++; // mas daño
            }
            else if (obj_actual == 4){ 
                state->player.rango_max += 100.0f; // mas rango
            }
            else if (obj_actual == 5){
                state->player.speed += 1.5f; // mas velocidad
            } 
            else if (obj_actual == 6){ 
                state->player.tiene_defensa = true; // activa la defensa 

                for (int f = 0; f < MAP_ROWS; f++) {
                    for (int c = 0; c < MAP_COL; c++) {
                        if (state->map.grid[f][c].objeto_tienda == 6) {
                            state->map.grid[f][c].objeto_tienda = 0;
                        }
                    }
                }
            }
            
            
 
            
                      // Hacemos que el objeto desaparezca
            state->map.grid[centro_y][centro_x].objeto_tienda = 0; 
            state->mensaje_tienda = NULL;
        }
    }

        state->mensaje_tienda = NULL;

        for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int check_x = centro_x + dx;
            int check_y = centro_y + dy;

            if (check_x >= 0 && check_x < MAP_COL && check_y >= 0 && check_y < MAP_ROWS) {
                int obj = state->map.grid[check_y][check_x].objeto_tienda;
                int precio = state->map.grid[check_y][check_x].precio_tienda;

                if (obj > 0) {
            switch (obj) {
            case 1: 
              state->mensaje_tienda = "Corazon (+1 Vida) - Precio: 3 monedas"; 
                break;
            case 2: 
               state->mensaje_tienda = "Municion extra (+10 Balas extra) - Precio: 3 monedas"; 
                break;
            case 3: 
                state->mensaje_tienda = "caipiriña (+1 Ataque) - Precio: 10 monedas"; 
                break;
            case 4: 
                state->mensaje_tienda = "Jugo de sandia (+100 Rango) - Precio: 10 monedas"; 
                break;
            case 5: 
                state->mensaje_tienda = "Limonada (+1.5 Vel) - Precio: 10 monedas"; 
                break;
            case 6: 
                state->mensaje_tienda = "Jugo de coco (escudo) - Precio: 10 monedas"; 
                break;
        }
      }
    }
  }
}

    for (int i = 0; i < MAXB; i++) {
        if (state->player.balas[i].active) {

            state->player.balas[i].x += state->player.balas[i].dx;
            state->player.balas[i].y += state->player.balas[i].dy;

            float avance = sqrt(state->player.balas[i].dx * state->player.balas[i].dx + state->player.balas[i].dy * state->player.balas[i].dy);
            state->player.balas[i].distancia_recorrida += avance;

            if (state->player.balas[i].distancia_recorrida > state->player.rango_max || check_collision(state, state->player.balas[i].x, state->player.balas[i].y) || state->player.balas[i].x < 0 || state->player.balas[i].x > 960 || state->player.balas[i].y < 0 || state->player.balas[i].y > 640) {
                
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
                    float e_size = state->enemigos[i].size;
                    
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

                    // CAMBIO DE IA
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
                    if (state->enemigos[i].tipo == 4) {
                        float dif_x = state->player.x - state->enemigos[i].x;
                        float dif_y = state->player.y - state->enemigos[i].y;
                        float distancia = sqrt(dif_x * dif_x + dif_y * dif_y);

                        // Si está a 180 píxeles o choca, se detiene a disparar (pasa al case 4)
                        if (distancia <= 180.0f || choco_x || choco_y) {
                            state->enemigos[i].estado_ia = 4;
                        }
                    }
                    break;
                }
        

                //tipo 2 
               case 2: { //IA DESATORO/PATRULLA
                    float e_size = state->enemigos[i].size;
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
                                state->enemigos[i].balasE[b].distancia_recorrida = 0.0f;
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
                case 4: {
                    if (state->enemigos[i].Ecooldown > 0){
                        state->enemigos[i].Ecooldown--;
                    }
                    else {
                        state->enemigos[i].Ecooldown = 60;
                        
                        //calculamos el angulo central hacia el jugador usando atan2
                        float dif_x = (state->player.x + 15.0f) - (state->enemigos[i].x + state->enemigos[i].size / 2.0f);
                        float dif_y = (state->player.y + 15.0f) - (state->enemigos[i].y + state->enemigos[i].size / 2.0f);
                        float angulo_centro = atan2(dif_y, dif_x); // devuelve el angulo en radianes
                        
                        //definimos la apertura en cono (0.3 radianes son aprox 17 grados)
                        float dispersion = 0.3f; 
                        float angulos[3] = {angulo_centro, angulo_centro - dispersion, angulo_centro + dispersion};
                        int balas_creadas = 0;
                        
                        //busca 3 espacios vacios para generar las 3 balas simultaneas
                        for(int b = 0; b < MAX_EB && balas_creadas < 3; b++){
                            if (!state->enemigos[i].balasE[b].active){
                                state->enemigos[i].balasE[b].active = true;

                                // salen del centro del jefe
                                state->enemigos[i].balasE[b].x = state->enemigos[i].x + (state->enemigos[i].size / 2);
                                state->enemigos[i].balasE[b].y = state->enemigos[i].y + (state->enemigos[i].size / 2);
                                state->enemigos[i].balasE[b].distancia_recorrida = 0.0f;
                                float vel_bala = 0.3f; 
                                state->enemigos[i].balasE[b].dx = cos(angulos[balas_creadas]) * vel_bala;
                                state->enemigos[i].balasE[b].dy = sin(angulos[balas_creadas]) * vel_bala;
                                
                                balas_creadas++;
                            }
                        }
                    }

                    float dif_x = state->player.x - state->enemigos[i].x;
                    float dif_y = state->player.y - state->enemigos[i].y;
                    float distancia = sqrt(dif_x * dif_x + dif_y * dif_y);

                    // Si te alejas a más de 200 píxeles, vuelve a perseguirte para no perderte de vista
                    if (distancia > 200.0f) {
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
                    if (state->player.balas[j].x >= state->enemigos[i].x && state->player.balas[j].x <= state->enemigos[i].x + state->enemigos[i].size && state->player.balas[j].y >= state->enemigos[i].y && state->player.balas[j].y <= state->enemigos[i].y + state->enemigos[i].size) {
                        
                        //destruimos la bala
                        state->player.balas[j].active = false;
                        
                        //le quitamos 1 de vida al enemigo
                        state->enemigos[i].vida -= state->player.daño_ataque;

                        //retroceso del enemigo
                        float fuerza_retroceso = 2.5f;
                        float empuje_x = state->player.balas[j].dx * fuerza_retroceso;
                        float empuje_y = state->player.balas[j].dy * fuerza_retroceso;

                        float e_size = state->enemigos[i].size;
                        float nuevo_x = state->enemigos[i].x + empuje_x;
                        float nuevo_y = state->enemigos[i].y + empuje_y;

                        if (!check_collision(state, nuevo_x, state->enemigos[i].y) && !check_collision(state, nuevo_x + e_size, state->enemigos[i].y) && !check_collision(state, nuevo_x, state->enemigos[i].y + e_size) && !check_collision(state, nuevo_x + e_size, state->enemigos[i].y + e_size)) {
                            // Si no hay pared, aplicamos el empuje en X
                            state->enemigos[i].x = nuevo_x; 
                        }

                        if (!check_collision(state, state->enemigos[i].x, nuevo_y) && !check_collision(state, state->enemigos[i].x + e_size, nuevo_y) && !check_collision(state, state->enemigos[i].x, nuevo_y + e_size) && !check_collision(state, state->enemigos[i].x + e_size, nuevo_y + e_size)) {
                            // Si no hay pared, aplicamos el empuje en Y
                            state->enemigos[i].y = nuevo_y; 
                        }
                        
                        //verifica si ya se quedó sin vidas
                        if (state->enemigos[i].vida <= 0) {
                            state->enemigos[i].active = false; //muere el enemigo

                            int grid_x = (int)(state->enemigos[i].x + 15) / TILE_SIZE;
                            int grid_y = (int)(state->enemigos[i].y + 15) / TILE_SIZE;
                            
                            if (grid_x >= 0 && grid_x < MAP_COL && grid_y >= 0 && grid_y < MAP_ROWS) {
                                state->map.grid[grid_y][grid_x].moneda = true;
                            }
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

            float avance = sqrt(state->enemigos[i].balasE[b].dx * state->enemigos[i].balasE[b].dx + state->enemigos[i].balasE[b].dy * state->enemigos[i].balasE[b].dy);
            state->enemigos[i].balasE[b].distancia_recorrida += avance;

            float rango_max_enemigo = 220.0f;

            //colision con paredes o fuera de pantalla
            if (state->enemigos[i].balasE[b].distancia_recorrida > rango_max_enemigo || check_collision(state, state->enemigos[i].balasE[b].x, state->enemigos[i].balasE[b].y) || state->enemigos[i].balasE[b].x < 0 || state->enemigos[i].balasE[b].x > 960 || state->enemigos[i].balasE[b].y < 0 || state->enemigos[i].balasE[b].y > 640) {
                state->enemigos[i].balasE[b].active = false;
            }
                //colision contra jugador si no es invulnerable
            else if (state->player.invu_timer == 0){
                if (state->enemigos[i].balasE[b].x >= state->player.x && state->enemigos[i].balasE[b].x <= state->player.x + 30 && state->enemigos[i].balasE[b].y >= state->player.y && state->enemigos[i].balasE[b].y <= state->player.y + 30) {
                    
                    state->enemigos[i].balasE[b].active = false;

                    if (state->player.tiene_defensa) {
                         if (state->player.turno_defensa) {
                               state->player.vida--; // Recibe daño
                         }
                         //va encendiendo y apagando el daño recibido para que se vea que le quita menos vida
                         state->player.turno_defensa = !state->player.turno_defensa;
                    }
                         else {
                             state->player.vida--; // daño normal si no tiene la bebida
                         }

                         state->player.invu_timer = 500; // 5 segundos de invunerabilidad

                    if (state->player.vida <= 0){
                        state->estadoapp = 2;
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
                    if (state->player.x < state->enemigos[i].x + state->enemigos[i].size && state->player.x + 30 > state->enemigos[i].x && state->player.y < state->enemigos[i].y + state->enemigos[i].size && state->player.y + 30 > state->enemigos[i].y){
                        if (state->player.tiene_defensa) {
                            if (state->player.turno_defensa) {
                                 state->player.vida--;
                            }
                         state->player.turno_defensa = !state->player.turno_defensa;
                        }
                          else {
                                state->player.vida--;
                           }

                    state->player.invu_timer = 500;

                        if (state->player.vida <= 0){
                            state->estadoapp = 2;
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

    //dibuja menu inicio
    if (state->estadoapp == 0) {
        al_draw_text(state->font, al_map_rgb(255, 255, 255), 480, 250, ALLEGRO_ALIGN_CENTER, "MI JUEGO INCREIBLE");
        al_draw_text(state->font, al_map_rgb(255, 255, 0), 480, 350, ALLEGRO_ALIGN_CENTER, "Presiona ENTER para Empezar");
        al_flip_display();
        return; // Retornamos para que no dibuje el nivel de fondo
    }

    else if (state->estadoapp == 2){
        al_draw_text(state->font, al_map_rgb(255, 0, 0), 480, 250, ALLEGRO_ALIGN_CENTER, "HAS MUERTO");
        al_draw_text(state->font, al_map_rgb(255, 255, 255), 480, 350, ALLEGRO_ALIGN_CENTER, "Presiona ENTER para Reintentar");
        al_flip_display();
        return;
    }


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
            else if (state->map.grid[fil][col].moneda) {
                if (state->sprite_moneda) {
                    al_draw_bitmap(state->sprite_moneda, col * TILE_SIZE, fil * TILE_SIZE, 0);
                }
            }
            else if (state->map.grid[fil][col].cumulo_monedas) {
                if (state->sprite_cumulo_monedas) {
                    al_draw_bitmap(state->sprite_cumulo_monedas, col * TILE_SIZE, fil * TILE_SIZE, 0);
                }
            }
            else if (state->map.grid[fil][col].estado_cofre == 2) {
                if (state->sprite_cofre_cerrado) {
                    al_draw_bitmap(state->sprite_cofre_cerrado, col * TILE_SIZE, fil * TILE_SIZE, 0);
                }
            }
            else if (state->map.grid[fil][col].estado_cofre == 3) {
                if (state->sprite_cofre_abierto) {
                    al_draw_bitmap(state->sprite_cofre_abierto, col * TILE_SIZE, fil * TILE_SIZE, 0);
                }
            }
            else if (state->map.grid[fil][col].objeto_tienda > 0) {
                int tipo = state->map.grid[fil][col].objeto_tienda;
                float x = col * TILE_SIZE;
                float y = fil * TILE_SIZE;

                // Dibujar el sprite correspondiente
                if (tipo == 1 && state->sprite_corazon){
                    al_draw_bitmap(state->sprite_corazon, x, y, 0); 
                }
                else if (tipo == 2 && state->sprite_objeto){ 
                    al_draw_bitmap(state->sprite_objeto, x, y, 0); 
                }
                else if (tipo == 3 && state->sprite_bebida_daño){ 
                    al_draw_bitmap(state->sprite_bebida_daño, x, y, 0); 
                }
                else if (tipo == 4 && state->sprite_bebida_rango){
                    al_draw_bitmap(state->sprite_bebida_rango, x, y, 0); 
                }
                else if (tipo == 5 && state->sprite_bebida_vel){
                    al_draw_bitmap(state->sprite_bebida_vel, x, y, 0); 
                }
                else if (tipo == 6 && state->sprite_bebida_def){
                    al_draw_bitmap(state->sprite_bebida_def, x, y, 0); 
                }
                
                al_draw_textf(state->font, al_map_rgb(255, 215, 0), x, y + 32, 0, "$%d", state->map.grid[fil][col].precio_tienda);
            }
        }
    }

    if (state->mensaje_tienda != NULL) {
        al_draw_text(state->font, al_map_rgb(255, 255, 0), 480, 580, ALLEGRO_ALIGN_CENTER, state->mensaje_tienda);
    }
    
    //dibuja los tipos de enemigos
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemigos[i].active) {
            if (state->enemigos[i].tipo == 1 && state->sprite_ene1) {
                al_draw_bitmap(state->sprite_ene1, state->enemigos[i].x, state->enemigos[i].y, 0);
            }
            if (state->enemigos[i].tipo == 2 && state->sprite_ene2) {
                al_draw_bitmap(state->sprite_ene2, state->enemigos[i].x, state->enemigos[i].y, 0);
            }
            if (state->enemigos[i].tipo == 3 && state->sprite_ene3) {
                al_draw_bitmap(state->sprite_ene3, state->enemigos[i].x, state->enemigos[i].y, 0);
            }
            if (state->enemigos[i].tipo == 4){
                al_draw_filled_rectangle(state->enemigos[i].x, state->enemigos[i].y, state->enemigos[i].x + state->enemigos[i].size, state->enemigos[i].y + state->enemigos[i].size, al_map_rgb(200, 0, 50)); // Rojo oscuro para el jefe
            }
            
        }
    }

    if (state->player.invu_timer == 0 || (state->player.invu_timer / 25) % 2 == 0) {
        
        ALLEGRO_BITMAP* hoja_activa = NULL;
        int max_frames = 1;

        if (state->player.estado_animacion == 1) {
            hoja_activa = state->sprite_jugador_run;
            max_frames = 10;
        } else {
            hoja_activa = state->sprite_jugador_idle;
            max_frames = 8;
        }
        if (hoja_activa != NULL) {
            // Calculamos automáticamente el ancho de 1 fotograma
            int ancho_frame = al_get_bitmap_width(hoja_activa) / max_frames;
            int alto_frame = al_get_bitmap_height(hoja_activa);

            // posición del corte en X
            int origen_x = state->player.frame_actual * ancho_frame;

            // determina la orientación usando IF
            int flags = 0;
            if (state->player.direccion == 2) {
                flags = ALLEGRO_FLIP_HORIZONTAL; // voltea si va a la izquierda
            }

            al_draw_bitmap_region(hoja_activa, origen_x, 0, ancho_frame, alto_frame, state->player.x, state->player.y, flags);
        }
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

    // barra de vida del jefeee
    for (int i = 0; i < MAX_ENEMIES; i++) {
        // busca si esta activo el jefe osea tipo 4
        if (state->enemigos[i].active && state->enemigos[i].tipo == 4) {
            
            //configuración del tamaño y posición
            float ancho_maximo = 400.0f; // ancho total de la barra en píxeles
            float alto_barra = 20.0f;    // grosor
            float x_inicial = 280.0f;    // posición en la derecha de la pantalla
            float y_inicial = 590.0f;    // misma altura que tus textos de UI

            // calculamos el porcentaje de vida (usando float para no perder decimales)
            // dividimos entre 30.0f porque es la vida máxima del jefe
            float porcentaje = (float)state->enemigos[i].vida / 30.0f; 
            float ancho_actual = ancho_maximo * porcentaje;

            // dibujamos el fondo (Gris oscuro para la vida perdida)
            al_draw_filled_rectangle(x_inicial, y_inicial, x_inicial + ancho_maximo, y_inicial + alto_barra, al_map_rgb(50, 50, 50));
            
            // dibujamos la barra de vida restante (Rojo)
            if (ancho_actual > 0) {
                al_draw_filled_rectangle(x_inicial, y_inicial, x_inicial + ancho_actual, y_inicial + alto_barra, al_map_rgb(200, 0, 50));
            }
            
            // dibujamos un marco estetico (Blanco)
            al_draw_rectangle(x_inicial, y_inicial, x_inicial + ancho_maximo, y_inicial + alto_barra, al_map_rgb(255, 255, 255), 2.0f);
            
            // texto para identificar la barra
            al_draw_text(state->font, al_map_rgb(255, 255, 255), x_inicial - 55, y_inicial + 5, 0, "JEFE:");
            
            break;
        }
    }

    al_draw_textf(state->font, al_map_rgb(255, 255, 255), 20, 550, 0, "BALAS: %d / 15         VIDAS: %d         MONEDAS: %d", state->player.municion, state->player.vida, state->player.monedas);
    
    al_flip_display();

}