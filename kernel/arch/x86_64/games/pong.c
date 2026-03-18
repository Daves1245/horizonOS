#include "pong.h"
#include <drivers/ac97.h>
#include <drivers/ps2k.h>
#include <drivers/graphics.h>

#include <math.h>
#include <rand.h>

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 100
#define BALL_RADIUS 10
/* speeds are in pixels per second; multiply by delta_ms then divide by 1000 */
#define PLAYER_SPEED 800

#define BALL_MAX_SPEED 500

static int screen_width, screen_height;
static int last_player_scored;

struct obj player1, player2, ball;

int score_player1;
int score_player2;

static void reset();

void pong_init(int width, int height) {
    screen_width = width;
    screen_height = height;

    player1 = (struct obj){
        0,
        screen_height / 2 - PADDLE_HEIGHT / 2,
        PADDLE_WIDTH,
        PADDLE_HEIGHT,
        0,
        0,
    };

    player2 = (struct obj){
        screen_width - PADDLE_WIDTH,
        screen_height / 2 - PADDLE_HEIGHT / 2,
        PADDLE_WIDTH,
        PADDLE_HEIGHT,
        0,
        0
    };

    ball = (struct obj){
        screen_width / 2 - BALL_RADIUS / 2,
        screen_height / 2 - BALL_RADIUS / 2,
        BALL_RADIUS,
        BALL_RADIUS,
        0,
        0
    };
}

void pong_handle_input() {
    // if no key is pressed, don't move
    player1.vel_y = 0;
    player2.vel_y = 0;

    if (is_key_pressed(KEY_W)) {
        player1.vel_y = -PLAYER_SPEED;
    }

    if (is_key_pressed(KEY_S)) {
        player1.vel_y = PLAYER_SPEED;
    }

    if (is_key_pressed(KEY_ARROW_UP)) {
        player2.vel_y = -PLAYER_SPEED;
    }

    if (is_key_pressed(KEY_ARROW_DOWN)) {
        player2.vel_y = PLAYER_SPEED;
    }
}

static int rects_overlap(struct obj *a, struct obj *b) {
    return a->x < b->x + b->width &&
           a->x + a->width > b->x &&
           a->y < b->y + b->height &&
           a->y + a->height > b->y;
}

static void pong_on_player_hit(struct obj *paddle) {
    /* set absolute x direction based on which side the paddle is on */
    if (paddle->x < screen_width / 2) {
        ball.vel_x = abs(ball.vel_x);
    } else {
        ball.vel_x = -abs(ball.vel_x);
    }

    /* transfer some paddle momentum to ball */
    ball.vel_y += paddle->vel_y / 2;
}

static void pong_on_wall_hit() {
    if (ball.y <= 0) {
        ball.vel_y = abs(ball.vel_y);
    } else if (ball.y + ball.height >= screen_height) {
        ball.vel_y = -abs(ball.vel_y);
    }
}

void pong_on_score(int player) {
    (void) player;

    // play score sound

    // halt while sound plays

    // add score and reset ball
    reset();
}

void pong_update(int delta) {
    player1.y += player1.vel_y * delta / 1000;
    player2.y += player2.vel_y * delta / 1000;

    ball.x += ball.vel_x * delta / 1000;
    ball.y += ball.vel_y * delta / 1000;

    /* wall collisions (top/bottom) */
    if (ball.y <= 0 || ball.y + ball.height >= screen_height) {
        pong_on_wall_hit();
    }

    /* paddle collisions */
    if (rects_overlap(&ball, &player1)) {
        pong_on_player_hit(&player1);
    }
    if (rects_overlap(&ball, &player2)) {
        pong_on_player_hit(&player2);
    }

    if (ball.x + ball.width <= 0) {
        pong_on_score(1);
    }

    if (ball.x - ball.width >= screen_width) {
        pong_on_score(2);
    }
}

void pong_start() {
    score_player1 = score_player2 = 0;
    reset();
}

static void reset() {
    ball.x = screen_width / 2 - ball.width / 2;
    ball.y = screen_height / 2 - ball.height / 2;

    if (last_player_scored == 1) {
        ball.vel_x = rand_range(1, BALL_MAX_SPEED);
    } else if (last_player_scored == 2) {
        ball.vel_x = -rand_range(1, BALL_MAX_SPEED);
    } else {
        ball.vel_x = rand_range(-BALL_MAX_SPEED, BALL_MAX_SPEED);
    }
    ball.vel_y = rand_range(-BALL_MAX_SPEED, BALL_MAX_SPEED);
}

void pong_draw() {
    gfx_clear_screen();
    gfx_fill_rect(player1.x, player1.y, player1.x + player1.width, player1.y + player1.height, rgb(0xff, 0xff, 0xff));
    gfx_fill_rect(player2.x, player2.y, player2.x + player2.width, player2.y + player2.height, rgb(0xff, 0xff, 0xff));
    gfx_fill_rect(ball.x, ball.y, ball.x + ball.width, ball.y + ball.height, rgb(0xff, 0xff, 0xff));
}

