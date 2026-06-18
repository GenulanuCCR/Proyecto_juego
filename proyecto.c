#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>

#define MAXB 10

typedef struct {
    float x, y;
    float dx, dy;
    bool active;
} bala;

typedef struct {
    float x, y;
    float speed;
    bala balas[MAXB];
} jugador;

typedef struct {
    int ancho;
    int largo;
} mapa;

typedef struct {
    jugador player;
    mapa map;
    bool keys[4];    // guardamos el estado de W, A, S, D aquí adentro
    bool game_over;
} GameState;

// DEFINICIÓN DE FUNCIONES

void init_game(GameState* state) {
    state->player.x = 320;
    state->player.y = 240;
    state->player.speed = 4.0f;
    
    for(int i = 0; i < MAXB; i++) {
        state->player.balas[i].active = false;
    }
    
    // inicializamos las teclas en falso
    for(int i = 0; i < 4; i++) {
        state->keys[i] = false;
    }
    
    state->map.ancho = 640;
    state->map.largo = 480;
    state->game_over = false;
}

void manejo_input(GameState* state, ALLEGRO_EVENT* event) {
    // 1 registro de movimiento (WASD)
    if(event->type == ALLEGRO_EVENT_KEY_DOWN) {
        switch(event->keyboard.keycode) {
            case ALLEGRO_KEY_W: state->keys[0] = true; break;
            case ALLEGRO_KEY_A: state->keys[1] = true; break;
            case ALLEGRO_KEY_S: state->keys[2] = true; break;
            case ALLEGRO_KEY_D: state->keys[3] = true; break;
        }

        // 2 registro de disparos (Flechas)
        float b_dx = 0, b_dy = 0;
        bool disparo = false;

        switch(event->keyboard.keycode) {
            case ALLEGRO_KEY_UP:    b_dy = -8;  disparo = true; break;
            case ALLEGRO_KEY_DOWN:  b_dy = 8;   disparo = true; break;
            case ALLEGRO_KEY_LEFT:  b_dx = -8;  disparo = true; break;
            case ALLEGRO_KEY_RIGHT: b_dx = 8;   disparo = true; break;
            case ALLEGRO_KEY_ESCAPE: state->game_over = true; break; // salir con ESC
        }

        if(disparo) {
            for(int i = 0; i < MAXB; i++) {
                if(!state->player.balas[i].active) {
                    state->player.balas[i].x = state->player.x + 15;
                    state->player.balas[i].y = state->player.y + 15;
                    state->player.balas[i].dx = b_dx;
                    state->player.balas[i].dy = b_dy;
                    state->player.balas[i].active = true;
                    break;
                }
            }
        }
    }
    else if(event->type == ALLEGRO_EVENT_KEY_UP) {
        switch(event->keyboard.keycode) {
            case ALLEGRO_KEY_W: state->keys[0] = false; break;
            case ALLEGRO_KEY_A: state->keys[1] = false; break;
            case ALLEGRO_KEY_S: state->keys[2] = false; break;
            case ALLEGRO_KEY_D: state->keys[3] = false; break;
        }
    }
}

void update_game(GameState* state) {
    // movimiento del jugador usando las teclas de la estructura
    if(state->keys[0]) state->player.y -= state->player.speed;
    if(state->keys[1]) state->player.x -= state->player.speed;
    if(state->keys[2]) state->player.y += state->player.speed;
    if(state->keys[3]) state->player.x += state->player.speed;

    // movimiento de las balas
    for(int i = 0; i < MAXB; i++) {
        if(state->player.balas[i].active) {
            state->player.balas[i].x += state->player.balas[i].dx;
            state->player.balas[i].y += state->player.balas[i].dy;

            // si salen de la pantalla se apagan
            if(state->player.balas[i].x < 0 || state->player.balas[i].x > 640 ||
               state->player.balas[i].y < 0 || state->player.balas[i].y > 480) {
                state->player.balas[i].active = false;
            }
        }
    }
}

void draw_game(GameState* state) {
    al_clear_to_color(al_map_rgb(0, 0, 0));
    
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


