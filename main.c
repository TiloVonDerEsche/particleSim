/*
Version: 0.0.5
C Standard: C17
Author: Tilo von Eschwege
*/

#include <stdio.h>
#include <Math.h>
#include <stdint.h>
#include <stdlib.h>

#include "E:\res\SDL3\include\SDL3\SDL.h"
#include "constants.h"
//#include "force.h"


typedef struct Vec2D {
  float x;
  float y;
} vec2D;

typedef struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t alpha; //transparency
} color;

typedef struct Ball
{
    vec2D pos;
    vec2D velo;
    vec2D accel;


    uint16_t width;
    uint16_t height;
    float density;
    float mass;

    float charge;
    color color;
} ball;



////////////////////////////////////////////////////////////////
//----------------------global variables----------------------//
////////////////////////////////////////////////////////////////
vec2D mouse;

int ball_num = 0;
int len_balls_arr = 100;
ball* balls;


uint8_t game_is_running = FALSE;

float delta_time = 0;
int last_frame_time = 0;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;


color proton_color = {255, 0, 0, 255};
color neutron_color = {0, 255, 0, 100};
color electron_color = {0, 0, 255, 255};

////////////////////////////////////////////////////////////////
//--------------------------Functions-------------------------//
////////////////////////////////////////////////////////////////
void init_ball(ball* b) {
  b->velo = (vec2D){0, 0};
  b->accel = (vec2D){0, 0};

  b->width = 5;
  b->height = 5;

  b->density = 1;
  b->mass = b->density * ((float)(b->width * b->height));
}

void spawn_ball(float x, float y, float charge, color color) {
  if (len_balls_arr <= ball_num + 1) {
    len_balls_arr *= 2;

    ball* new_balls = realloc(balls, sizeof(ball) * len_balls_arr);
    if (new_balls == NULL) {
      fprintf(stderr, "realloc of balls failed!\nball_num=%d, len_balls_arr=%d"
        ,ball_num, len_balls_arr);
      exit(-1);
    }
    balls = new_balls;
    //printf("Reallocated balls! New Size:%d\n",len_balls_arr);
  }

  ball* b = &balls[ball_num];

  b->pos = (vec2D){x, y};
  b->charge = charge;
  b->color = color;
  //printf("Spawning Ball at (%d, %d), with index: %d\n",x, y, ball_num);

  init_ball(b);

  ball_num++;
}

/******************************************
* Applies the x and y speed
* to the x and y coordiantes of the balls
* Handles collision of the balls
* and the window border.
******************************************/
void check_borders(ball* b) {
  //left or right
  if ((b->pos.x > (WINDOW_WIDTH - b->width)) || (b->pos.x < 0))
  {
      b->velo.x *= -1;
  }
  //bottom or top
  if ((b->pos.y > (WINDOW_HEIGHT - b->height)) || (b->pos.y < 0))
  {
      b->velo.y *= -1;
  }
}

void update_ball_movement(float dt)
{
    ball* b;
    for (int i = 0; i < ball_num; i++)
    {
        b = &balls[i];
        //check_borders(b);

        b->velo.x += b->accel.x * dt;
        b->velo.y += b->accel.y * dt;

        b->pos.x += b->velo.x * dt;
        b->pos.y += b->velo.y * dt;

    }
}

float magnitude(vec2D* vec) {
  return sqrt(pow(vec->x,2) + pow(vec->y,2));
}

vec2D normalize(vec2D* vec, float mag) {
  vec2D vec_norm;

  if (mag > 0) {
    vec_norm.x = vec->x / mag;
    vec_norm.y = vec->y / mag;
  } else {
    vec_norm.x = 0;
    vec_norm.y = 0;
  }

  return vec_norm;
}

float dotp(vec2D* vec1, vec2D* vec2) {
  return vec1->x*vec2->x + vec1->y*vec2->y;
}

float get_gravity(ball* b1, ball* b2, float d) {
  return G * (b1->mass * b2->mass) / (pow(d,2) + pow(SOFTENING_FACTOR,2));

}

float get_coulomb(ball* b1, ball* b2, float d) {
  return COULOMB_CONSTANT * b1->charge * b2->charge / (pow(d,2) + pow(SOFTENING_FACTOR,2));
}

float magnetic_field(ball* b, float velo ,float theta, float d) {
  return MAGNETIC_FIELD_CONTSTANT * b->charge * velo * sin(theta) / (pow(d,2) + pow(SOFTENING_FACTOR,2));
}

void apply_forces()
{
    float gravity;
    float coulomb_force;
    float magnetic_force;

    vec2D net_force;

    vec2D vec;
    vec2D vec_norm;
    float d;

    vec2D velo;
    vec2D velo_norm;
    float mag_velo;

    float theta;

    ball* b1;
    ball* b2;
    for (int i = 0; i < ball_num; i++)
    {
        b1 = &balls[i];
        net_force.x = 0.0;
        net_force.y = 0.0;

        for (int j = 0; j < ball_num; j++)
        {
            if (i != j) {
              b2 = &balls[j];
              vec.x = b2->pos.x - b1->pos.x;
              vec.y = b2->pos.y - b1->pos.y;

              d = magnitude(&vec); //distance btw b1 & b2
              if (d != 0) {
                vec_norm = normalize(&vec, d);

                velo = b2->velo;
                mag_velo = magnitude(&velo);
                velo_norm = normalize(&velo, mag_velo);

                theta = acos(fmax(1, fmin(-1, dotp(&vec_norm, &velo_norm))));


                  if (d > MIN_ACTION_DISTANCE) {
                    // gravity = get_gravity(b1,b2,d);
                    // //the normalized vector is multiplied with the gravitational force
                    // net_force.x +=  vec_norm.x * gravity;
                    // net_force.y +=  vec_norm.y * gravity;


                    //Lorentz Force
                    coulomb_force = get_coulomb(b1,b2,d);
                    net_force.x += vec_norm.x * coulomb_force;
                    net_force.y += vec_norm.y * coulomb_force;

                    magnetic_force = magnetic_field(b2, mag_velo, theta, d);
                    net_force.x += vec_norm.x * magnetic_force;
                    net_force.y += vec_norm.y * magnetic_force;
                  }
              }

            }


        }

        if (b1->mass != 0) {
          b1->accel.x = net_force.x / b1->mass; // F = m * a -> a = F / m
          b1->accel.y = net_force.y / b1->mass;
        }


    }
}



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
        "GravitySim", //window title
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
                ball_num = 0;
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
                spawn_ball(mouse.x, mouse.y, 1, proton_color);
                break;
              case 2:
                spawn_ball(mouse.x, mouse.y, 0, neutron_color);
                break;
              case 3:
                spawn_ball(mouse.x, mouse.y, -1, electron_color);
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
  balls = calloc(len_balls_arr, sizeof(ball));
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



    apply_forces();
    update_ball_movement(delta_time);



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
    for (int i = 0; i < ball_num; i++) {

        //printf("rendering ball at (%f,%f)\n",balls[i].pos.x,balls[i].pos.y);
        b = &balls[i];
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


    SDL_RenderPresent(renderer);

}






/***************************
* Quits the renderer,
* SDL and closes the window.
***************************/
void destroy_window()
{
    free(balls);
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
