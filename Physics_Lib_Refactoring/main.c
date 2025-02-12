/*
Version: 0.0.5
C Standard: C17
Author: Tilo von Eschwege
*/

#include <stdio.h>
#include <Math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "E:\res\SDL3\include\SDL3\SDL.h"
#include "physics.h"
//#include "force.h"

#define WINDOW_WIDTH 2560
#define WINDOW_HEIGHT 1440

#define FPS 144
#define FRAME_TARGET_TIME (1000/FPS)

#define TRUE 1
#define FALSE 0

char* window_title = "Physics_Lib_Test";
uint8_t game_is_running = FALSE;

float delta_time = 0;
int last_frame_time = 0;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

char* density_label;
float spawn_density = 1.0;

bl* balls;
vec2D mouse;


/**********************************
* Creates the window and renderer,
* While handling potential errors.
**********************************/
int initialize_window()
{
    int initErrC = SDL_Init(SDL_INIT_VIDEO);
    if (initErrC != 1)
    {
        fprintf(stderr, "An error occured, while initializing SDL.\nError Code: %d\nSDL Error:%s\n",initErrC,SDL_GetError());

        return FALSE;
    }

    window = SDL_CreateWindow(
        window_title, //window title
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_FULLSCREEN
    );

    if (!window)
    {
        fprintf(stderr, "An error occured, while creating the window.\nSDL Error:%s\n",SDL_GetError());
        return FALSE;
    }


    // int numRenders = SDL_GetNumRenderDrivers();
    // printf("NumRenderDrivers:%d\n",
    // numRenders);
    //
    // puts("\nAvailable Renders:\n");
    // for(int i = 0; i < numRenders; i++) {
    //   printf("%s\n",SDL_GetRenderDriver(i));
    // }

    renderer = SDL_CreateRenderer(window,"gpu");//driver code, display number; -1 -> default

    if (!renderer)
    {
        fprintf(stderr, "An error occured, while creating the SDL-Renderer.\nSDL Error:%s\n",SDL_GetError());
        return FALSE;
    }

    return TRUE;
}


/*********************
* Handles user-input.
*********************/
void process_input()
{
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type)
    {
        case SDL_EVENT_QUIT: //window's x button is clicked
            game_is_running = FALSE;
            break;

        case SDL_EVENT_KEY_DOWN: //keypress

            switch(event.key.key)
            {
              case SDLK_ESCAPE:
                game_is_running = FALSE;
                break;

              case SDLK_O:
                balls->num = 0;
                break;

              case SDLK_UP:
                spawn_density++;
                break;
              case SDLK_DOWN:
                spawn_density--;
                break;
            }
            break;
            //printf("%d\n",event.key.key);

        case SDL_EVENT_MOUSE_MOTION:
            mouse.x = (uint16_t)event.motion.x;
            mouse.y = (uint16_t)event.motion.y;

            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            //printf("mouse=(%d, %d)\n",(int)mouse.x, (int)mouse.y);
            //printf("Mouse button:%d\n", event.button.button);

            switch(event.button.button) {
              case 1:
                spawn_ball(mouse.x, mouse.y, 1, proton_color, &balls);
                break;
              case 2:
                spawn_ball(mouse.x, mouse.y, 0, neutron_color, &balls);
                break;
              case 3:
                spawn_ball(mouse.x, mouse.y, -1, electron_color, &balls);
                break;
            }
            break;

    }
}


/************************
* Gets called only once.
*************************/
void setup()
{

  balls = malloc(sizeof(bl) + 5 * sizeof(ball));  // 20 balls
  balls->num = 0;
  balls->len = 5;


  density_label = calloc(9 + sizeof(float),1); //"Density: %f"-> 9 + sizeof(float)
}


/************************
* Applies physics
* To the game objects.
*************************/
void update()
{
    //delay, so that the capped framerate is reached (and not overshoot).
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);

    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME)
    {
        SDL_Delay(time_to_wait);
    }


    delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;

    last_frame_time = SDL_GetTicks();



    apply_forces(balls);
    update_ball_movement(delta_time, balls);



}


/*******************************
* Re-/Draws the window,
* With all of the game-objects.
*******************************/
void render()
{
    //background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //R, G, B, Alpha
    SDL_RenderClear(renderer);

    ball* b;
    for (int i = 0; i < balls->num; i++) {

        //printf("rendering ball at (%f,%f)\n",balls[i].pos.x,balls[i].pos.y);
        b = &balls->arr[i];
        SDL_FRect ball_rect =
        {
            (int)b->pos.x,
            (int)b->pos.y,
            b->width,
            b->height
        };

        SDL_SetRenderDrawColor(renderer, b->color.r, b->color.g, b->color.b, b->color.alpha); //4 colors(ugly): i % 3 * 255, (i + 1) % 3 * 255, (i + 2) % 3 * 255, 255
        SDL_RenderFillRect(renderer, &ball_rect);
    }


    sprintf(density_label, "Density: %f", spawn_density);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDebugText(renderer, 14, 65, density_label);

    SDL_RenderPresent(renderer);

}






/***************************
* Quits the renderer,
* SDL and closes the window.
***************************/
void destroy_window()
{
    free(balls);
    free(density_label);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


/*********************************
* Initializes the window,
* Calls the setup function
* And starts the game-loop,
* With the process_input(),
* update() and render() function.
* When the game-loop terminates,
* The window gets destroyed.
*
* @param argc, @param args
*********************************/
void game() {
  printf("Game is running...\n");


  game_is_running = initialize_window();
  setup();
  while (game_is_running) //Game Loop
  {
      process_input();
      update();
      render();

  }

  destroy_window();
}

int main(int argc, char* args[])
{
    game();
    return 0;
}

/*
* The click of the mouse should spawn a ball at that position.
* That way the user doesn't need to input the ball_num
*/
