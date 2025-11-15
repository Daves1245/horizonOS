#include <i386/drivers/shell/builtins/maze.h>
#include <i386/drivers/timer.h>
#include <kernel/tty.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Eller's Algorithm implementation (archived for debugging)
// This implementation has the set_id tracking logic

struct cell_eller {
    int type;
    int set_id;
    int right_wall;
    int bottom_wall;
};

static struct cell_eller maze_eller[MAZE_HEIGHT][MAZE_WIDTH];
static int next_set_id_eller;

extern uint32_t random_range(uint32_t max);

void generate_maze_eller() {
    next_set_id_eller = 1;

    // Initialize first row with unique sets
    for (int x = 0; x < MAZE_WIDTH; x++) {
        maze_eller[0][x].type = CORRIDOR;
        maze_eller[0][x].set_id = next_set_id_eller++;
        maze_eller[0][x].right_wall = 1;
        maze_eller[0][x].bottom_wall = 1;
    }

    // Process each row except the last
    for (int y = 0; y < MAZE_HEIGHT - 1; y++) {
        // Step 1: Randomly join adjacent cells in same row (horizontal passages)
        for (int x = 0; x < MAZE_WIDTH - 1; x++) {
            // Only merge if in different sets
            if (maze_eller[y][x].set_id != maze_eller[y][x + 1].set_id) {
                // 50% chance to remove wall
                if (random_range(100) < 50) {
                    maze_eller[y][x].right_wall = 0;

                    // Merge sets - update all cells with old set to new set
                    int old_set = maze_eller[y][x + 1].set_id;
                    int new_set = maze_eller[y][x].set_id;
                    for (int i = 0; i < MAZE_WIDTH; i++) {
                        if (maze_eller[y][i].set_id == old_set) {
                            maze_eller[y][i].set_id = new_set;
                        }
                    }
                }
            }
        }

        // Step 2: Create vertical connections to next row
        // First, find all unique sets in current row
        int unique_sets[MAZE_WIDTH];
        int num_unique_sets = 0;

        for (int x = 0; x < MAZE_WIDTH; x++) {
            int set = maze_eller[y][x].set_id;
            int already_found = 0;
            for (int i = 0; i < num_unique_sets; i++) {
                if (unique_sets[i] == set) {
                    already_found = 1;
                    break;
                }
            }
            if (!already_found) {
                unique_sets[num_unique_sets++] = set;
            }
        }

        // For each unique set, ensure at least one vertical connection
        for (int s = 0; s < num_unique_sets; s++) {
            int current_set = unique_sets[s];
            int has_connection = 0;

            // Find all cells in this set and randomly connect some
            for (int x = 0; x < MAZE_WIDTH; x++) {
                if (maze_eller[y][x].set_id == current_set) {
                    // Random chance for vertical connection, but ensure at least one
                    int should_connect = random_range(100) < 40;

                    if (should_connect || !has_connection) {
                        maze_eller[y][x].bottom_wall = 0;
                        maze_eller[y + 1][x].type = CORRIDOR;
                        maze_eller[y + 1][x].set_id = current_set;
                        maze_eller[y + 1][x].right_wall = 1;
                        maze_eller[y + 1][x].bottom_wall = 1;
                        has_connection = 1;
                    }
                }
            }
        }

        // Step 3: Assign new sets to cells in next row that don't have connections
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (maze_eller[y + 1][x].type != CORRIDOR) {
                maze_eller[y + 1][x].type = CORRIDOR;
                maze_eller[y + 1][x].set_id = next_set_id_eller++;
                maze_eller[y + 1][x].right_wall = 1;
                maze_eller[y + 1][x].bottom_wall = 1;
            }
        }
    }

    // Final row: connect all different sets horizontally
    int last_row = MAZE_HEIGHT - 1;
    for (int x = 0; x < MAZE_WIDTH - 1; x++) {
        if (maze_eller[last_row][x].set_id != maze_eller[last_row][x + 1].set_id) {
            // Force remove wall to ensure full connectivity
            maze_eller[last_row][x].right_wall = 0;

            // Merge sets
            int old_set = maze_eller[last_row][x + 1].set_id;
            int new_set = maze_eller[last_row][x].set_id;
            for (int i = 0; i < MAZE_WIDTH; i++) {
                if (maze_eller[last_row][i].set_id == old_set) {
                    maze_eller[last_row][i].set_id = new_set;
                }
            }
        }
    }

    // Copy to main maze structure (would need to be implemented)
    // This is just archived code for reference
}
