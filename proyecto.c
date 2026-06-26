#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>
#include <stdio.h>
#define MAXB 15
#define TILE_SIZE 32
#define MAP_ROWS 15
#define MAP_COL 20

typedef struct {
bool W;
bool A;
bool S;
bool D;
bool up;
bool down;
bool left;
bool right;
}teclado;

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
    bala balas[MAXB];
} jugador;

typedef struct {
    int grid[MAP_ROWS][MAP_COL];
} mapa;

typedef struct {
    jugador player;
    mapa map;
    teclado keys;    // guardamos el estado de W0, A1, S2, D3 aquí adentro
    bool game_over;

} GameState;

// DEFINICIÓN DE FUNCIONES
bool check_collision(GameState* state, float x, float y) {
    int grid_x = (int)x / TILE_SIZE;
    int grid_y = (int)y / TILE_SIZE;
    
 if (grid_x < 0 || grid_x >= MAP_COL || grid_y < 0 || grid_y >= MAP_ROWS) {
     return true; // fuera de los límites del mapa
 }
 return state->map.grid[grid_y][grid_x] == 1; // 1 representa un obstáculo
}

void load_map(GameState* state, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("¡Error! No se pudo abrir el archivo %s. Creando mapa vacío.\n", filename);
        // Si no encuentra el archivo, llena los bordes manualmente para que no crasheé
        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COL; c++) {
                if (r == 0 || r == MAP_ROWS - 1 || c == 0 || c == MAP_COL - 1) state->map.grid[r][c] = 1;
                else state->map.grid[r][c] = 0;
            }
        }
        return;
    }

    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COL; c++) {
            fscanf(file, "%d", &state->map.grid[r][c]);
        }
    }
    fclose(file);
}

void init_game(GameState* state) {

    load_map(state, "mapa.txt");

    state->player.x = 40;
    state->player.y = 40;
    state->player.speed = 3.0f;
    state->player.cooldown = 0;
    
    for(int i = 0; i < MAXB; i++) {
        state->player.balas[i].active = false;
    }
    
    // inicializamos las teclas en falso
    for(int i = 0; i < 7; i++) {
        state->keys.A = false;
        state->keys.W = false;
        state->keys.S = false;
        state->keys.D = false;
        state->keys.up = false;
        state->keys.down = false;
        state->keys.left = false;
        state->keys.right = false;
        state->player.disparo = false;
    }
    
    state->game_over = false;
}

void manejo_input(GameState* state, ALLEGRO_EVENT* event) {
 

    // 1 registro de movimiento (WASD)
    if(event->type == ALLEGRO_EVENT_KEY_DOWN) {
        switch(event->keyboard.keycode) {
            case ALLEGRO_KEY_W: state->keys.W = true; break;
            case ALLEGRO_KEY_A: state->keys.A = true; break;
            case ALLEGRO_KEY_S: state->keys.S = true; break;
            case ALLEGRO_KEY_D: state->keys.D = true; break;
            case ALLEGRO_KEY_ESCAPE: state->game_over = true; break; //salir con esc
        
        // Mantenemos presionadas las flechas
            case ALLEGRO_KEY_UP:    state->keys.up = true;    break;
            case ALLEGRO_KEY_DOWN:  state->keys.down = true;  break;
            case ALLEGRO_KEY_LEFT:  state->keys.left = true;  break;
            case ALLEGRO_KEY_RIGHT: state->keys.right = true; break;
        }
    }

    else if(event->type == ALLEGRO_EVENT_KEY_UP) {
            switch(event->keyboard.keycode) {
             case ALLEGRO_KEY_W: state->keys.W = false; break;
             case ALLEGRO_KEY_A: state->keys.A = false; break;
             case ALLEGRO_KEY_S: state->keys.S = false; break;
             case ALLEGRO_KEY_D: state->keys.D = false; break;

             // Al soltar la tecla, detenemos el fuego automático
             case ALLEGRO_KEY_UP:    state->keys.up = false;    break;
             case ALLEGRO_KEY_DOWN:  state->keys.down = false;  break;
             case ALLEGRO_KEY_LEFT:  state->keys.left = false;  break;
             case ALLEGRO_KEY_RIGHT: state->keys.right = false; break;
        }
    }

}

    void update_game(GameState* state) {

        float next_x = state->player.x;
        float next_y = state->player.y;
        float p_size = 30; // tamaño del jugador
        float shoot_dx = 0;
        float shoot_dy = 0;
        bool intentando_disparar = false;

     // movimiento del jugador usando las teclas de la estructura
        if(state->keys.W) next_y -= state->player.speed;
        if(state->keys.A) next_x -= state->player.speed;
        if(state->keys.S) next_y += state->player.speed;
        if(state->keys.D) next_x += state->player.speed;

        if(state->player.cooldown > 0) {
            state->player.cooldown--;
        }

        if(state->keys.up)    { shoot_dy = -8; intentando_disparar = true; }
        if(state->keys.down)  { shoot_dy = 8;  intentando_disparar = true; }
        if(state->keys.left)  { shoot_dx = -8; intentando_disparar = true; }
        if(state->keys.right) { shoot_dx = 8;  intentando_disparar = true; }

             if(intentando_disparar && state->player.cooldown == 0) {
                for(int i = 0; i < MAXB; i++) {
                    if(!state->player.balas[i].active) {
                    state->player.balas[i].x = state->player.x + 15;
                    state->player.balas[i].y = state->player.y + 15;
                    state->player.balas[i].dx = shoot_dx;
                    state->player.balas[i].dy = shoot_dy;
                    state->player.balas[i].active = true;
                    state->player.cooldown = 15;
                    break;
                }
            }
            state->player.disparo = false;
          }
    

          // chequeo de colisiones para el jugador EJE X (verificar las 4 esquinas)
          if (!check_collision(state, next_x, state->player.y) && !check_collision(state, next_x + p_size, state->player.y) && !check_collision(state, next_x, state->player.y + p_size) && !check_collision(state, next_x + p_size, state->player.y + p_size)) {
          state->player.x = next_x;
         }

          // chequeo de colisiones para el jugador EJE Y (verificar las 4 esquinas)
          if (!check_collision(state, state->player.x, next_y) && !check_collision(state, state->player.x + p_size, next_y) && !check_collision(state, state->player.x, next_y + p_size) &&!check_collision(state, state->player.x + p_size, next_y + p_size)) {
           state->player.y = next_y;
         }

         // movimiento de las balas
            for(int i = 0; i < MAXB; i++) {
        if(state->player.balas[i].active) {
            state->player.balas[i].x += state->player.balas[i].dx;
            state->player.balas[i].y += state->player.balas[i].dy;

            // Si chocan con un muro o salen de pantalla, se apagan y quedan libres para volver a dispararse
            if (check_collision(state, state->player.balas[i].x, state->player.balas[i].y) ||
                state->player.balas[i].x < 0 || state->player.balas[i].x > 640 ||
                state->player.balas[i].y < 0 || state->player.balas[i].y > 480) {
                
                state->player.balas[i].active = false;
            }
        }
    }
}

void draw_game(GameState* state) {
    al_clear_to_color(al_map_rgb(20,20,20));

    // 1. DIBUJAR EL MAPA
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COL; c++) {
            if (state->map.grid[r][c] == 1) {
                // Dibujamos un bloque azul/gris para las paredes
                al_draw_filled_rectangle(c * TILE_SIZE, r * TILE_SIZE, 
                                         (c + 1) * TILE_SIZE, (r + 1) * TILE_SIZE, 
                                         al_map_rgb(70, 90, 120));
                // Línea de borde para que se vean los bloques individuales
                al_draw_rectangle(c * TILE_SIZE, r * TILE_SIZE, 
                                  (c + 1) * TILE_SIZE, (r + 1) * TILE_SIZE, 
                                  al_map_rgb(40, 50, 70), 1);
            }
        }
    }
    
    // dibujar jugador
    al_draw_filled_rectangle(state->player.x, state->player.y, state->player.x + 30, state->player.y + 30, al_map_rgb(0, 255, 0));

    // dibujar balas
    for(int i = 0; i < MAXB; i++) {
        if(state->player.balas[i].active) {
            al_draw_filled_circle(state->player.balas[i].x, state->player.balas[i].y, 4, al_map_rgb(255, 255, 0));
        }
    }
    
    al_flip_display();
}



int main() {
    al_init();
    al_install_keyboard();
    al_init_primitives_addon();

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(640, 480);
    
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    bool redraw = true;

    GameState gs;
    init_game(&gs);

    al_start_timer(timer); // ¡Encendemos el temporizador!

    while (!gs.game_over) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if(event.type == ALLEGRO_EVENT_KEY_DOWN || event.type == ALLEGRO_EVENT_KEY_UP) {
            manejo_input(&gs, &event);
        }
        else if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            gs.game_over = true;
        }
    
        if(event.type == ALLEGRO_EVENT_TIMER) {
            update_game(&gs);
            redraw = true;
        }

        if(redraw && al_is_event_queue_empty(queue)) {
            draw_game(&gs);
            redraw = false;
        }
    }

    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}