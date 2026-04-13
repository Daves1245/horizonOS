#include "pong.h"
#include <drivers/ac97.h>
#include <drivers/ps2k.h>
#include <drivers/graphics.h>
#include <drivers/timer.h>

#include <math.h>
#include <rand.h>

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 100
#define BALL_RADIUS 10
/* speeds are in pixels per second; multiply by delta_ms then divide by 1000 */
#define PLAYER_SPEED 1000

#define BALL_MAX_SPEED 500

#define NET_WIDTH 15
#define NET_HEIGHT (screen_height / 20)

/* events must happen at least 10 ms apart */
#define EVENT_DEBOUNCE_TICKS 10

/* ~60 fps frame budget */
#define PONG_FRAME_MS 16

uint32_t last_event = 0;

static int screen_width, screen_height;
static int last_player_scored;

struct obj player1, player2, ball;

int score_player1;
int score_player2;

virt_addr_t audio_pong_wall_start;
virt_addr_t audio_pong_wall_end;
virt_addr_t audio_pong_paddle_start;
virt_addr_t audio_pong_paddle_end;
virt_addr_t audio_pong_score_start;
virt_addr_t audio_pong_score_end;


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

    audio_pong_wall_start = virt_to_phys((virt_addr_t)&_audio_pong_wall_start);
    audio_pong_wall_end = virt_to_phys((virt_addr_t)&_audio_pong_wall_end);
    audio_pong_paddle_start = virt_to_phys((virt_addr_t)&_audio_pong_paddle_start);
    audio_pong_paddle_end = virt_to_phys((virt_addr_t)&_audio_pong_paddle_end);
    audio_pong_score_start = virt_to_phys((virt_addr_t)&_audio_pong_score_start);
    audio_pong_score_end = virt_to_phys((virt_addr_t)&_audio_pong_score_end);
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
    // debounce events
    uint32_t now = timer_ticks();
    if (now - last_event >= EVENT_DEBOUNCE_TICKS) {
        last_event = now;

        /* set absolute x direction based on which side the paddle is on */
        if (paddle->x < screen_width / 2) {
            ball.vel_x = abs(ball.vel_x);
        } else {
            ball.vel_x = -abs(ball.vel_x);
        }

        /* transfer some paddle momentum to ball */
        ball.vel_y += paddle->vel_y / 2;

        ac97_setup_bdl(audio_pong_paddle_start, audio_pong_paddle_end);
        ac97_start_playback();
    }
}

static void pong_on_wall_hit() {
    uint32_t now = timer_ticks();
    if (now - last_event >= EVENT_DEBOUNCE_TICKS) {
        last_event = now;

        if (ball.y <= 0) {
            ball.vel_y = abs(ball.vel_y);
        } else if (ball.y + ball.height >= screen_height) {
            ball.vel_y = -abs(ball.vel_y);
        }

        ac97_setup_bdl(audio_pong_wall_start, audio_pong_wall_end);
        ac97_start_playback();
    }
}

void pong_on_score(int player) {
    uint32_t now = timer_ticks();
    if (now - last_event >= EVENT_DEBOUNCE_TICKS) {
        last_event = now;

        // play score sound
        ac97_setup_bdl(audio_pong_score_start, audio_pong_score_end);
        ac97_start_playback();

        // add score and reset ball
        if (player == 1) score_player1++;
        if (player == 2) score_player2++;
        reset();
    }
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

/* reset ball + paddle state for a new round; scores untouched. */
void pong_start_round() {
    reset();
}

void pong_start() {
    score_player1 = score_player2 = 0;
    last_player_scored = 0;
    pong_start_round();

    enum gfx_target prev_target = gfx_get_target();
    gfx_set_target(GFX_TARGET_BACKBUFFER);

    uint32_t last = timer_ticks();
    while (score_player1 < PONG_WIN_SCORE && score_player2 < PONG_WIN_SCORE) {
        uint32_t frame_start = timer_ticks();
        int delta = (int)(frame_start - last);
        last = frame_start;

        pong_handle_input();
        pong_update(delta);
        pong_draw();
        gfx_render();

        /* pace to ~60 fps so delta is coarse enough for integer
         * velocity math (vel * delta / 1000) to actually advance. */
        while (timer_ticks() - frame_start < PONG_FRAME_MS)
            asm volatile("hlt");
    }

    gfx_set_target(prev_target);
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

#define FONT_SCALE 4

void draw_score(int score, int x, int y) {
    int digit = score % 10;
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (display[digit][row][col]) {
                gfx_fill_rect(
                    x + col * FONT_SCALE,
                    y + row * FONT_SCALE,
                    x + (col + 1) * FONT_SCALE,
                    y + (row + 1) * FONT_SCALE,
                    rgb(0xff, 0xff, 0xff));
            }
        }
    }
}

void pong_draw() {
    gfx_clear_screen();

    // player 1
    gfx_fill_rect(player1.x, player1.y, player1.x + player1.width, player1.y + player1.height, rgb(0xff, 0xff, 0xff));

    // player 2
    gfx_fill_rect(player2.x, player2.y, player2.x + player2.width, player2.y + player2.height, rgb(0xff, 0xff, 0xff));

    // ball
    gfx_fill_rect(ball.x, ball.y, ball.x + ball.width, ball.y + ball.height, rgb(0xff, 0xff, 0xff));

    // scores
    draw_score(score_player1, screen_width / 4 - (5 * FONT_SCALE) / 2, 20);
    draw_score(score_player2, 3 * screen_width / 4 - (5 * FONT_SCALE) / 2, 20);

    // net (lines down the middle)
    // draw 1, skip 2 to "draw" an invisible net / break
    for (int i = 0; i < screen_height; i += 2 * NET_HEIGHT) {
        gfx_fill_rect(screen_width / 2 - NET_WIDTH / 2, i, screen_width / 2 + NET_WIDTH / 2, i + NET_HEIGHT, rgb(0xff, 0xff, 0xff));
    }
}

