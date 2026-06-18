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
bool keys[4];
bool game_over;
} GameState;

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

    bool done = false;
    bool redraw;

    GameState gs;
    init_game(&gs);

    while (!done)
    {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if(event.type == ALLEGRO_EVENT_KEY_DOWN || event.type == ALLEGRO_EVENT_KEY_UP) {
        manejo_input(&gs, &event);
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

void init_game(GameState* state) {
    state->player.x = 320;
    state->player.y = 240;
    state->player.speed = 3.0f;
    for(int i = 0; i < MAXB; i++) {
        state->player.balas[i].active = false;
    }
    state->map.ancho = 20;
    state->map.largo = 15;
    state->game_over = false;
}

void update_game(GameState* state, bool keys[4]) {
    if(keys[0]) state->player.y -= state->player.speed;
    if(keys[1]) state->player.x -= state->player.speed;
    if(keys[2]) state->player.y += state->player.speed;
    if(keys[3]) state->player.x += state->player.speed;

    for(int i = 0; i < MAXB; i++) {
        if(state->player.balas[i].active) {
            state->player.balas[i].x += state->player.balas[i].dx;
            state->player.balas[i].y += state->player.balas[i].dy;

            if(state->player.balas[i].x < 0 || state->player.balas[i].x > 640 ||
               state->player.balas[i].y < 0 || state->player.balas[i].y > 480) {
                state->player.balas[i].active = false;
            }
        }
    }
}
void draw_game(GameState* state) {
    al_clear_to_color(al_map_rgb(0, 0, 0));
    
    al_draw_filled_rectangle(state->player.x, state->player.y, state->player.x + 30, state->player.y + 30, al_map_rgb(0, 255, 0));

    for(int i = 0; i < MAXB; i++) {
        if(state->player.balas[i].active) {
            al_draw_filled_circle(state->player.balas[i].x, state->player.balas[i].y, 4, al_map_rgb(255, 255, 0));
        }
    }
    
    al_flip_display();
}

void manejo_input(GameState* state, ALLEGRO_EVENT* event) {
    if(event->type == ALLEGRO_EVENT_KEY_DOWN) {
        switch(event->keyboard.keycode) {
            case ALLEGRO_KEY_W: keys[0] = true; break;
            case ALLEGRO_KEY_A: keys[1] = true; break;
            case ALLEGRO_KEY_S: keys[2] = true; break;
            case ALLEGRO_KEY_D: keys[3] = true; break;
        }
    }
    else if(event->type == ALLEGRO_EVENT_KEY_UP) {
        switch(event->keyboard.keycode) {
            case ALLEGRO_KEY_W: keys[0] = false; break;
            case ALLEGRO_KEY_A: keys[1] = false; break;
            case ALLEGRO_KEY_S: keys[2] = false; break;
            case ALLEGRO_KEY_D: keys[3] = false; break;
        }
    }
}








