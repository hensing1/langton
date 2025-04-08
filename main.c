#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define GRID_X 50 
#define GRID_Y 40
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
} State;

typedef char Cell;

typedef struct {
    Cell** cells;
    size_t size_x;
    size_t size_y;
} Grid;


char step(State* ant_state, Grid* grid);
void print_grid(Grid* grid);


int main(int argc, char** argv) {

    // initialize grid
    Grid* grid = malloc(sizeof(Grid));
    grid->size_x = GRID_X;
    grid->size_y = GRID_Y;
    grid->cells = malloc(sizeof(Cell*) * grid->size_y);

    for (size_t i = 0; i < grid->size_y; i++) {
        grid->cells[i] = calloc(grid->size_x, sizeof(Cell));
    }

    // initialize ant
    State* ant_state = malloc(sizeof(State));
    *ant_state = (State) {
        .pos = (Pos) {
            .x = ANT_START_X,
            .y = ANT_START_Y
        },
        .dir = ANT_START_DIR
    };

    // let it rip
    print_grid(grid);
    char valid = 1;
    while (valid) {
        valid = step(ant_state, grid);
        printf("\x1b[%luA", grid->size_y);
        print_grid(grid);
        usleep(1000 * DELAY_MILLIS);
    }

    return 0;
}


char step(State* ant_state, Grid* grid) {
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
