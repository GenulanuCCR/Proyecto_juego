#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h> // Para dibujar el cubito
#include <stdbool.h>

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
    bool redraw = true;
    
    float x = 320, y = 240;
    
    bool keys[4] = {false, false, false, false}; 

    al_start_timer(timer);

    while(!done) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);


        if(event.type == ALLEGRO_EVENT_TIMER) {
            if(keys[0]) y -= 5;
            if(keys[1]) x -= 5;
            if(keys[2]) y += 5;
            if(keys[3]) x += 5;
            redraw = true;
        }
        else if(event.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch(event.keyboard.keycode) {
                case ALLEGRO_KEY_W: keys[0] = true; break;
                case ALLEGRO_KEY_A: keys[1] = true; break;
                case ALLEGRO_KEY_S: keys[2] = true; break;
                case ALLEGRO_KEY_D: keys[3] = true; break;
            }
        }
        else if(event.type == ALLEGRO_EVENT_KEY_UP) {
            switch(event.keyboard.keycode) {
                case ALLEGRO_KEY_W: keys[0] = false; break;
                case ALLEGRO_KEY_A: keys[1] = false; break;
                case ALLEGRO_KEY_S: keys[2] = false; break;
                case ALLEGRO_KEY_D: keys[3] = false; break;
                case ALLEGRO_KEY_ESCAPE: done = true; break;
            }
        }
        else if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            done = true;
        }

        if(redraw && al_is_event_queue_empty(queue)) {
            al_clear_to_color(al_map_rgb(0, 0, 0));
            
            al_draw_filled_rectangle(x, y, x + 30, y + 30, al_map_rgb(0, 255, 0)); 
            
            al_flip_display();
            redraw = false;
        }
    }

    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}
