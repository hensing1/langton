#include <curses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define GRID_X 80 
#define GRID_Y 60
#define ANT_START_X GRID_X/2
#define ANT_START_Y GRID_Y/2
#define ANT_START_DIR E
#define DELAY_MILLIS 100


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
    size_t screen_width;
    size_t screen_height;
} Simulation;

const char* symbols[] = {
    "██", "░░"
};


void init_ui(Simulation* sim);
void print_ant(Grid* grid, AntState* ant_state);
void print_cell(Grid* grid, Pos position);
void print_grid(Grid* grid);
char step(Simulation* sim);
void quit(int);


int main(int argc, char** argv) {

    // initialize ncurses
    initscr();
    noecho(); // no input echo
    cbreak(); // no input buffering
    curs_set(false); // no cursor
    nodelay(stdscr, true); // getch() is non-blocking
    keypad(stdscr, true); // special keys (arrows etc)

    // interrupt handler
    signal(SIGINT, quit);

    Simulation* sim = malloc(sizeof(Simulation));
    sim->ips = 25;
    sim->iteration = 0;
    getmaxyx(stdscr, sim->screen_height, sim->screen_width);


    // initialize grid
    Grid* grid = malloc(sizeof(Grid));
    grid->size_x = sim->screen_width / 2;
    grid->size_y = sim->screen_height - 7;
    grid->cells = malloc(sizeof(Cell*) * grid->size_y);

    for (size_t i = 0; i < grid->size_y; i++) {
        grid->cells[i] = calloc(grid->size_x, sizeof(Cell));
    }

    // initialize ant
    AntState* ant_state = malloc(sizeof(AntState));
    *ant_state = (AntState) {
        .pos = (Pos) {
            .x = grid->size_x / 2,
            .y = grid->size_y / 2
        },
        .dir = ANT_START_DIR
    };

    sim->grid = grid;
    sim->ant_state = ant_state;

    // let it rip
    char valid = 1;
    while (valid) {
        Pos old_ant_pos = ant_state->pos;
        valid = step(sim);
        print_cell(grid, old_ant_pos);
        if (valid) {
            print_ant(grid, ant_state);
        }
        usleep(1000000 / sim->ips);
    }

    return 0;
}


void print_header(Simulation* sim) {
    if (sim->running) {
        mvprintw(0, 0, "[p]ause \t[q]uit");
    }
    else {
        mvprintw(0, 0, "[r]un \t[s]tep \t[q]uit");
    }

    mvprintw(2, 0, "Speed: %d it./sec", sim->ips);
    mvprintw(3, 0, "[+]: increase speed \t[-]: decrease speed");

    mvprintw(5, 0, "Iteration: %d", sim->iteration);
}


void init_ui(Simulation* sim) {

    print_header(sim);

    
}


char step(AntState* ant_state, Grid* grid) {
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
    int new_x, new_y;
    switch (ant_state->dir) {
        case N:
            new_x = ant_state->pos.x;
            new_y = ant_state->pos.y - 1;
        break;
        case E:
            new_x = ant_state->pos.x + 1;
            new_y = ant_state->pos.y;
        break;
        case S:
            new_x = ant_state->pos.x;
            new_y = ant_state->pos.y + 1;
        break;
        case W:
            new_x = ant_state->pos.x - 1;
            new_y = ant_state->pos.y;
        break;
    }

    if (new_x < 0 || new_x >= grid->size_x || new_y < 0 || new_y >= grid->size_y) {
        return 0;
    }
    ant_state->pos.x = new_x;
    ant_state->pos.y = new_y;
    return 1;

}


void print_grid(Grid* grid) {
    for (size_t y = 0; y < grid->size_y; y++) {
        for (size_t x = 0; x < grid->size_x; x++) {
            if (grid->cells[y][x] == 1) {
                printf("░░");
            }
            else {
                printf("██");
            }
        }
        printf("\n");
    }
}


void quit(int sig) {
    endwin();
    exit(0);
}
