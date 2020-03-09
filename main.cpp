#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include "cp_functions.h"

#define WindowTitle "Breakout 62"
#define WindowWidth 800
#define WindowHeight 700
#define MaxLevel 5

//variable for check my current level
int level = 1;

Sound hit_paddle_sound, hit_brick_sound;
Sound hit_top_sound, end_sound;
Texture paddle_texture, ball_texture;
Texture brick_texture, background_texture;
Font big_font, small_font;

// Structure for storing info for objects, i.e. Paddle, Brick, Ball.
class Object {
public:
   float pos_x, pos_y;
   float vel_x, vel_y;
   float width, height;
   bool active;

   //constructor on Object
   Object(){
      Object(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false);
   }
   Object(float in_pos_x, float in_pos_y, float in_vel_x, float in_vel_y, 
          float in_width, float in_height, bool in_active)
    : pos_x(in_pos_x), pos_y(in_pos_y), vel_x(in_vel_x), vel_y(in_vel_y), 
      width(in_width), height(in_height), active(in_active) {}

   //method
   // Collision Detection between objects a and b
   int collide(Object b)
   {
      if (this->pos_x + this->width < b.pos_x || b.pos_x + b.width < this->pos_x ||
         this->pos_y + this->height < b.pos_y || b.pos_y + b.height < this->pos_y)
         return False;
      else
         return True;
   }

};

// Initial routine to load sounds, textures, and fonts.
int game_init()
{
   hit_paddle_sound = cpLoadSound("hitDown.wav");
   hit_brick_sound = cpLoadSound("hitUp.wav");
   hit_top_sound = cpLoadSound("hitTop.wav");
   end_sound = cpLoadSound("theEnd.wav");

   paddle_texture = cpLoadTexture("paddle.png");
   ball_texture = cpLoadTexture("ball.png");
   brick_texture = cpLoadTexture("brick.png");
   background_texture = cpLoadTexture("background.png");

   big_font = cpLoadFont("THSarabun.ttf", 60);
   small_font = cpLoadFont("THSarabun.ttf", 30);

   return hit_paddle_sound && hit_brick_sound && hit_top_sound && end_sound &&
      paddle_texture && ball_texture && brick_texture && background_texture &&
      big_font && small_font;
}

// Special Objects
Texture ghost_texture;
Object ghost = {0, WindowHeight / 2, 0, 0, 50, 50, True};
int ready_to_swap=1;
pthread_t ghost_thread;

void *ghostBot(void *ptr)
{  double timestep=0;
   float vel_x=5;

   while (True) {
      if (!ready_to_swap) {
         usleep(1000);
         continue;
      }
      if (ghost.pos_x < 0) {
         ghost.pos_x = 0;
         vel_x *= -1.;
      }
      else if (ghost.pos_x > WindowWidth - ghost.width) {
         ghost.pos_x = WindowWidth - ghost.width;
         vel_x *= -1.;
      }
      ghost.pos_x += vel_x;
      ghost.pos_y = WindowHeight/2 + 50*sin(8*M_PI*timestep/(WindowWidth - ghost.width));
      timestep += fabs(vel_x);
      ready_to_swap = 0;
   }
}

void waitQuitEvent(bool &running, Event event){
   while (True) {
      cbEventListener(&event);
      if (event.type == QUIT ||
            event.type == KEYUP && event.key.keysym.sym == K_ESCAPE) {
         running = False;
         break;
      }
   }
}

void nextLevel(int &n_hits, int goal, bool &running, Event event){
   if(n_hits >= goal){
      cpPlaySound(end_sound);
      n_hits = 0;
      level++;
      //running = false for recreate map
      running=False;
   }
   //if go to MaxLevel, End while loop level
   if(level > MaxLevel) waitQuitEvent(running, event);
}

//checking game ending
void endGame(float ball_y, float ball_width, int &n_hits, int n_bricks, int h_bricks, bool &running, Event event){
   if (ball_y + ball_width > WindowHeight) {
      cpPlaySound(end_sound);
      cpDrawText(255, 255, 0, 400, 350, "จบเกมจบกัน GG", big_font, 1);
      cpSwapBuffers();
      waitQuitEvent(running, event);
   }
}

int main(int argc, char *args[])
{
   enum {
      BALL_VEL_Y = -5,
      PADDLE_VEL_X = 7
   };
   bool running;
   int h_bricks = 6, n_bricks = 15, n_hits = 0, score = 0;
   char msg[80];
   Object bricks[h_bricks][n_bricks];
   Object ball = {WindowWidth / 2 - 12, 350, 0, BALL_VEL_Y, 24, 24, True};
   Object paddle = {WindowWidth / 2 - 62, WindowHeight - 50, 0, 0, 124, 18, True};
   Event event;

   if (!cpInit(WindowTitle, WindowWidth, WindowHeight)) {
      fprintf(stderr, "Window initialization failed!\n");
      exit(1);
   }

   if (!game_init()) {
      fprintf(stderr, "Game initialization failed!\n");
      exit(1);
   }

   ghost_texture = cpLoadTexture("ghost.png");
   pthread_create(&ghost_thread, NULL, ghostBot, (void *)NULL);

   int lap_current = 1;
   //While Loop level -> Check in case we not pass to next level but lose, lap_current and level will not equal
   while(lap_current == level){

      //create new bricks
      for(int h = 0, y = 80; h<h_bricks; h++){
         int x = -10;
         int bricks_width = 55;
         int bricks_height = 18;
         for (int n = 0; n < n_bricks; n++) {
               bricks[h][n].pos_x = x;
               bricks[h][n].pos_y = y;
               bricks[h][n].width = bricks_width;
               bricks[h][n].height = bricks_height;
               bricks[h][n].active = True;
               x += bricks[h][n].width;
         }
         y += bricks_height;
      }

      running = True;
      //Loop in each Level
      while (running) {
         cpClearScreen();
         cpDrawTexture(255, 255, 255,
                     0, 0, WindowWidth, WindowHeight, background_texture);
         cpDrawTexture(255, 255, 255,
                     paddle.pos_x, paddle.pos_y, paddle.width, paddle.height, paddle_texture);
         cpDrawTexture(255, 255, 255,
                     ball.pos_x, ball.pos_y, ball.width, ball.height, ball_texture);
         cpDrawTexture(255, 255, 255,
                     ghost.pos_x, ghost.pos_y, ghost.width, ghost.height, ghost_texture);
         ready_to_swap = 1;

         for(int h = 0; h<h_bricks; h++){
            for (int n = 0; n < n_bricks; n++) {
               if (bricks[h][n].active)
                  cpDrawTexture(255, 255, 255,
                              bricks[h][n].pos_x, bricks[h][n].pos_y, bricks[h][n].width, bricks[h][n].height,
                              brick_texture);
            }
         }
         sprintf(msg, "คะแนน %d | Level %d", score, level);
         cpDrawText(255, 255, 255, 3, 3, msg, small_font, 0);
         
         //check END game
         endGame(ball.pos_y, ball.width, n_hits, n_bricks, h_bricks, running, event);
         cpSwapBuffers();
         
         //paddle check event
         while (cbEventListener(&event)) {
            if (event.type == QUIT ||
               event.type == KEYUP && event.key.keysym.sym == K_ESCAPE) {
               running = False;
               break;
            }

            if (event.type == KEYDOWN) {
               if (event.key.keysym.sym == K_LEFT)
                  paddle.vel_x -= 1.2;
               if (event.key.keysym.sym == K_RIGHT)
                  paddle.vel_x += 1.2;
            }
            else if (event.type == KEYUP) {
               if (event.key.keysym.sym == K_LEFT)
                  paddle.vel_x = 0;
               if (event.key.keysym.sym == K_RIGHT)
                  paddle.vel_x = 0;
            }
         }
         paddle.pos_x += PADDLE_VEL_X*(paddle.vel_x==0?0:(paddle.vel_x>0?1:-1));

         if (paddle.pos_x < 0)
            paddle.pos_x = 0;
         if (paddle.pos_x + paddle.width > WindowWidth)
            paddle.pos_x = WindowWidth - paddle.width;

         //move ball object
         ball.pos_x += ball.vel_x;
         ball.pos_y += ball.vel_y;

         if (ball.pos_x < 0 || ball.pos_x + ball.width > WindowWidth)
            ball.vel_x = -ball.vel_x;

         if (ball.pos_y < 0) {
            cpPlaySound(hit_top_sound);
            ball.vel_y = -ball.vel_y;
         }

         //check ball hit brinks
         for(int h = 0; h<h_bricks; h++){
            for (int n = 0; n < n_bricks; n++) {
               if (bricks[h][n].active && ball.collide(bricks[h][n])) {
                  cpPlaySound(hit_brick_sound);
                  ball.vel_y = -ball.vel_y;
                  bricks[h][n].active = False;
                  n_hits++;
                  score += 10;
                  nextLevel(n_hits, (n_bricks*h_bricks), running, event);
                  break;
               }
            }
         }

         //check ball hit paddle
         if (ball.collide(paddle)) {
            cpPlaySound(hit_paddle_sound);
            ball.vel_x = ball.vel_x+paddle.vel_x;
            ball.vel_y = -ball.vel_y;
         }

         cpDelay(10);
      }
      lap_current++;
   }

   cpCleanUp();
   return 0;
}