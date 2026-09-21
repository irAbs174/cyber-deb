#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

/* Debian swirl. Still red at rest; cyan scan, spark, and glitch while it runs. */
static const char *logo[] = {
    "       _,met$$$$$gg.",
    "    ,g$$$$$$$$$$$$$$$P.",
    "  ,g$$P\"     \"\"\"Y$$.\".",
    " ,$$P'              `$$$.",
    "',$$P       ,ggs.     `$$b:",
    "`d$$'     ,$P\"'   .    $$$",
    " $$P      d$'     ,    $$P",
    " $$:      $$.   -    ,d$$'",
    " $$;      Y$b._   _,d$P",
    " Y$$.    `.`\"Y$$$$P\"'",
    " `$$b      \"-.__",
    "  `Y$$",
    "   `Y$$.",
    "     `$$b.",
    "       `Y$$b.",
    "          `\"Y$b._",
    "              `\"\"\"",
};

#define ROWS ((int)(sizeof logo / sizeof logo[0]))

static volatile sig_atomic_t stop;

static void on_stop(int sig)
{
    (void)sig;
    stop = 1;
}

static int is_body(char c)
{
    return c == '$' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static void set_rgb(int r, int g, int b)
{
    printf("\033[38;2;%d;%d;%dm", r, g, b);
}

static int glyph_count(void)
{
    int n = 0;
    int r;

    for (r = 0; r < ROWS; r++) {
        const char *p;

        for (p = logo[r]; *p; p++) {
            if (*p != ' ')
                n++;
        }
    }
    return n;
}

static const char glitch_set[] = "#%&@/\\|*";

static void draw(int frame, int glyphs, int glitch_row)
{
    int scan = frame % (ROWS + 3);
    int head = (frame * 3) % glyphs;
    int seen = 0;
    int r;

    for (r = 0; r < ROWS; r++) {
        const char *p;
        int col = 0;
        int dist = r - scan;

        if (dist < 0)
            dist = -dist;
        if (r == glitch_row)
            putchar(' ');

        for (p = logo[r]; *p; p++, col++) {
            int trail;
            char ch = *p;

            if (ch == ' ') {
                putchar(' ');
                continue;
            }

            trail = head - seen;
            if (trail < 0)
                trail += glyphs;
            seen++;

            if (r == glitch_row && ((col + frame) & 3) == 0) {
                set_rgb(255, 40, 180);
                putchar(glitch_set[(frame + col) % (int)(sizeof glitch_set - 1)]);
                continue;
            }

            if (trail == 0) {
                fputs("\033[1m", stdout);
                set_rgb(180, 255, 255);
                putchar(ch);
                fputs("\033[22m", stdout);
            } else if (trail < 5) {
                set_rgb(30, 220 - trail * 35, 230);
                putchar(ch);
            } else if (dist == 0) {
                set_rgb(255, 80, 120);
                putchar(ch);
            } else if (dist == 1) {
                set_rgb(210, 20, 70);
                putchar(ch);
            } else if (is_body(ch)) {
                set_rgb(168, 0, 48);
                putchar(ch);
            } else {
                set_rgb(255, 176, 186);
                putchar(ch);
            }
        }
        fputs("\033[0m\033[K\n", stdout);
    }
}

int main(void)
{
    int frame = 0;
    int glyphs;
    int glitch_row = -1;

    glyphs = glyph_count();
    if (glyphs <= 0)
        return 1;

    if (!isatty(STDOUT_FILENO)) {
        draw(0, glyphs, -1);
        return 0;
    }

    signal(SIGINT, on_stop);
    signal(SIGTERM, on_stop);
    srand((unsigned)time(NULL));
    fputs("\033[?25l", stdout);

    while (!stop) {
        if (frame > 0)
            printf("\033[%dA", ROWS);

        if ((frame % 14) == 0)
            glitch_row = rand() % ROWS;
        else if ((frame % 14) == 2)
            glitch_row = -1;

        draw(frame, glyphs, glitch_row);
        fflush(stdout);
        {
            struct timespec ts = {0, 50000000L};

            if (nanosleep(&ts, NULL) != 0 && stop)
                break;
        }
        frame++;
    }

    printf("\033[%dA", ROWS);
    draw(0, glyphs, -1);
    fputs("\033[?25h", stdout);
    fflush(stdout);
    return 0;
}
