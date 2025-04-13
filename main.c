#define _XOPEN_SOURCE_EXTENDED 1 // garbage for enabling wide chars for ncurses

#define ANT_START_DIR N
#define HEADER_SIZE 5  // lines needed for info at top of the screen
#define RED_FOREGROUND COLOR_PAIR(1)

#define max(a,b) (a) > (b) ? (a) : (b);

#include <ncursesw/ncurses.h>

#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <wchar.h>

typedef struct {
    int x;
    int y;
} Pos;

typedef enum {
    N, E, S, W
} Dir;

typedef struct {
    Pos pos;
    Dir dir;
} AntState;

typedef enum {
    WHITE, BLACK
} Cell;

typedef struct {
    Cell** cells;
    size_t size_x;
    size_t size_y;
} Grid;

typedef struct {
    Grid* grid;
    AntState* ant_state;
    unsigned int iteration;
    unsigned int ips; // iterations per second
    bool running;
} Simulation;

const wchar_t* symbols[] = {
    L"██", L"░░"
};

typedef enum {
    NOOP, TOGGLE_RUN, STEP, INC_SPEED, DEC_SPEED, RESET, QUIT
} Command;

volatile sig_atomic_t interrupted = 0;

bool ant_pos_valid(Simulation* sim);
void init_ncurses();
void init_sim(Simulation* sim);
void init_ui(Simulation* sim);
void free_sim_content(Simulation* sim);
bool handle_command(Simulation* sim, Command comm);
Grid* make_grid();
Command get_user_command(bool async);
void update_ant(Grid* grid, Pos old_ant_pos, Pos new_ant_pos, bool valid);
void print_cell(Grid* grid, Pos pos);
void print_grid(Grid* grid);
void print_header(Simulation* sim);
void step(Simulation* sim);
void quit(int);


int main(int argc, char** argv) {

    init_ncurses();

    // interrupt handler
    signal(SIGINT, quit);

    Simulation* sim = malloc(sizeof(Simulation));
    init_sim(sim);
    sim->ips = 25;
    init_ui(sim);

    while (!interrupted) {

        Pos old_ant_pos = sim->ant_state->pos;

        Command comm = get_user_command(sim->running);
        if (comm == QUIT) { break; }
        bool valid = handle_command(sim, comm);

        print_header(sim);
        update_ant(sim->grid, old_ant_pos, sim->ant_state->pos, valid);

        refresh();
        if (sim->running) {
            usleep(1000000 / sim->ips);
        }
    }

    free_sim_content(sim);
    free(sim);
    endwin();
    return 0;
}


///----- USER INPUT -----

Command get_user_command(bool async) {

    nodelay(stdscr, async); // if async, getch() is non-blocking
    int ch = getch();

    if (ch == ERR) {
        return NOOP;
    }
    if (ch == 'p') {
        return TOGGLE_RUN;
    }
    if (ch == 's') {
        return STEP;
    }
    if (ch == '+') {
        return INC_SPEED;
    }
    if (ch == '-') {
        return DEC_SPEED;
    }
    if (ch == 'r') {
        return RESET;
    }
    if (ch == 'q') {
        return QUIT;
    }

    return NOOP;
}


///----- SIMULATION -----

void init_sim(Simulation* sim) {
    sim->grid = make_grid();

    sim->ant_state = malloc(sizeof(AntState));
    *(sim->ant_state) = (AntState) {
        .pos = (Pos) {
            .x = sim->grid->size_x / 2,
            .y = sim->grid->size_y / 2
        },
        .dir = ANT_START_DIR
    };

    sim->iteration = 0;
    sim->running = false;
}


bool handle_command(Simulation* sim, Command comm) {

    bool step_allowed = ant_pos_valid(sim);

    switch (comm) {
    case TOGGLE_RUN:
        if (step_allowed) {
            sim->running ^= true;
        }
        break;
    case STEP:
        if (!sim->running && step_allowed) {
            step(sim);
        }
        break;
    case INC_SPEED:
        sim->ips++;
        break;
    case DEC_SPEED:
        sim->ips = max(1, (int)sim->ips - 1);
        break;
    case RESET:
        free_sim_content(sim);
        init_sim(sim);
        init_ui(sim);
        break;
    case QUIT:
        return false;
    case NOOP:
        break;
    }

    if (sim->running && step_allowed) {
        step(sim);
    }

    return ant_pos_valid(sim);
}


bool ant_pos_valid(Simulation* sim) {
    return (
        sim->ant_state->pos.x >= 0 && sim->ant_state->pos.x < sim->grid->size_x &&
        sim->ant_state->pos.y >= 0 && sim->ant_state->pos.y < sim->grid->size_y
    );
}


Grid* make_grid() {

    int screen_width, screen_height;
    getmaxyx(stdscr, screen_height, screen_width);

    Grid* grid = malloc(sizeof(Grid));
    grid->size_x = screen_width / 2;
    grid->size_y = screen_height - HEADER_SIZE;
    grid->cells = malloc(sizeof(Cell*) * grid->size_y);


    for (size_t i = 0; i < grid->size_y; i++) {
        grid->cells[i] = calloc(grid->size_x, sizeof(Cell));
    }

    return grid;
}


void step(Simulation* sim) {

    Grid* grid = sim->grid;
    AntState* ant_state = sim->ant_state;

    sim->iteration++;

    //scan 
    Cell current_cell = grid->cells[ant_state->pos.y][ant_state->pos.x];

    //turn
    if (current_cell == 0) {
        ant_state->dir++;
    }
    else {
        ant_state->dir--;
    }
    ant_state->dir %= 4;

    //flip
    grid->cells[ant_state->pos.y][ant_state->pos.x] = !current_cell;

    //move
    switch (ant_state->dir) {
    case N:
        ant_state->pos.y--;
        break;
    case E:
        ant_state->pos.x++;
        break;
    case S:
        ant_state->pos.y++;
        break;
    case W:
        ant_state->pos.x--;
        break;
    }
}


///----- PRINTING TO SCREEN -----

void init_ncurses() {
    initscr();
    noecho(); // no input echo
    cbreak(); // no input buffering
    curs_set(false); // no cursor
    nodelay(stdscr, true); // getch() is non-blocking
    keypad(stdscr, true); // special keys (arrows etc)
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    setlocale(LC_ALL, ""); // for unicode characters
}


void init_ui(Simulation* sim) {

    clear();
    print_header(sim);
    print_grid(sim->grid);
    update_ant(sim->grid, (Pos) {.x = 0, .y = 0}, sim->ant_state->pos, true);
    refresh();

}


void print_header(Simulation* sim) {

    move(1, 0);
    clrtoeol();
    move(2, 0);
    clrtoeol();
    move(4,0);
    clrtoeol();

    bool out_of_bounds = !ant_pos_valid(sim);

    attron(A_BOLD);
    mvprintw(0, 0, "==== Langton's Ant ====");
    attroff(A_BOLD);
    mvprintw(1, 0, "[p]lay/[p]ause");

    if (sim->running) { attron(A_DIM); }
    mvprintw(2, 0, "[s]tep");
    attroff(A_DIM);

    mvprintw(1, 23, "[+]: inc. speed");
    mvprintw(2, 23, "[-]: dec. speed");

    mvprintw(1, 46, "[q]uit");
    mvprintw(2, 46, "[r]eset");

    char* run_status =
        out_of_bounds ?
            "Status: out of bounds" :
            sim->running ? "Status: running" : "Status: paused";
    mvprintw(4, 0, "%s", run_status);

    mvprintw(4, 23, "Iteration: %d", sim->iteration);

    mvprintw(4, 46, "Speed: %d it./sec", sim->ips);
}


void print_grid(Grid* grid) {
    for (size_t y = 0; y < grid->size_y; y++) {
        for (size_t x = 0; x < grid->size_x; x++) {
            print_cell(grid, (Pos) {.x = x, .y = y});
        }
        printf("\n");
    }
}


void print_cell(Grid* grid, Pos pos) {
    int screen_x = pos.x * 2;
    int screen_y = pos.y + HEADER_SIZE;

    mvaddwstr(screen_y, screen_x, symbols[grid->cells[pos.y][pos.x]]);
}


void update_ant(Grid* grid, Pos old_ant_pos, Pos new_ant_pos, bool valid) {
    print_cell(grid, old_ant_pos);
    if (valid) {
        attron(RED_FOREGROUND);
        print_cell(grid, new_ant_pos);
        attroff(RED_FOREGROUND);
    }
}


///----- EXIT -----

void free_sim_content(Simulation* sim) {
    for (int y = 0; y < sim->grid->size_y; y++) {
        free(sim->grid->cells[y]);
    }
    free(sim->grid->cells);
    free(sim->grid);
    free(sim->ant_state);
}

void quit(int sig) {
    interrupted = 1;
}
