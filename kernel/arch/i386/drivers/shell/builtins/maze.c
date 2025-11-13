#include <i386/drivers/shell/builtins/maze.h>
#include <i386/drivers/timer.h>
#include <kernel/tty.h>
#include <kernel/rand.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <i386/common/vga.h>

/*
 * maze.c - generate, solve, and display a randmoly generated maze
 * we use a straightforward backtracker
 * TODO make interactive. since we don't have processes yet, this will entail
 * creating a state machine that switches between the shell and maze screen.
 */

struct cell maze[MAZE_HEIGHT][MAZE_WIDTH];
char display_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
int next_set_id;

void maze_init() {
    next_set_id = 1;
    clear_display_buffer();

    seed_rng(timer_ticks());

    // all cells in maze start as corridors with walls between them
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            maze[y][x].type = CORRIDOR;
            maze[y][x].right_wall = 1;
            maze[y][x].bottom_wall = 1;
        }
    }
}

void clear_display_buffer() {
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            display_buffer[y][x] = ' ';
        }
    }
}

void draw_help_text() {
    const char* help_text;
    help_text = "(g) generate - (s) solve - (d) display ";
    int len = strlen(help_text);
    int start_x = (DISPLAY_WIDTH - len) / 2;

    for (int i = 0; i < len && start_x + i < DISPLAY_WIDTH; i++) {
        display_buffer[0][start_x + i] = help_text[i];
    }
}

void draw_maze() {
    clear_display_buffer();
    draw_help_text();

    // Each maze cell needs 2x2 display space: 1 for cell + 1 for walls
    // Cell at maze[y][x] maps to display positions:
    // - Cell content at (y*2+1, x*2)
    // - Right wall at (y*2+1, x*2+1) if right_wall=1
    // - Bottom wall at (y*2+2, x*2) if bottom_wall=1
    // - Corner at (y*2+2, x*2+1) if both walls exist

    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            int cell_display_y = y * 2 + 1; // +1 for help text
            int cell_display_x = x * 2;

            // Skip if out of bounds
            if (cell_display_y >= DISPLAY_HEIGHT || cell_display_x >= DISPLAY_WIDTH) continue;

            // 1. Draw the cell content
            char cell_char;
            switch (maze[y][x].type) {
                case CORRIDOR: cell_char = ' '; break;
                case VISITED: cell_char = 'v'; break;
                case PATH: cell_char = 'p'; break;
                case SOURCE: cell_char = 'S'; break;
                case SINK: cell_char = 'E'; break;
                default: cell_char = ' '; break;
            }
            display_buffer[cell_display_y][cell_display_x] = cell_char;

            // 2. Draw right wall (vertical wall between this cell and cell to the right)
            if (x < MAZE_WIDTH - 1 && cell_display_x + 1 < DISPLAY_WIDTH) {
                if (maze[y][x].right_wall) {
                    display_buffer[cell_display_y][cell_display_x + 1] = '#';
                } else {
                    // No wall = corridor continues, inherit cell type from adjacent cells
                    char wall_char = ' ';

                    // If either adjacent cell is PATH or VISITED, show that in the connecting space
                    if (maze[y][x].type == PATH || (x + 1 < MAZE_WIDTH && maze[y][x + 1].type == PATH)) {
                        wall_char = 'p';
                    } else if (maze[y][x].type == VISITED || (x + 1 < MAZE_WIDTH && maze[y][x + 1].type == VISITED)) {
                        wall_char = 'v';
                    }

                    display_buffer[cell_display_y][cell_display_x + 1] = wall_char;
                }
            }

            // 3. Draw bottom wall (horizontal wall between this cell and cell below)
            if (y < MAZE_HEIGHT - 1 && cell_display_y + 1 < DISPLAY_HEIGHT) {
                if (maze[y][x].bottom_wall) {
                    display_buffer[cell_display_y + 1][cell_display_x] = '#';
                } else {
                    // No wall = corridor continues, inherit cell type from adjacent cells
                    char wall_char = ' ';

                    // If either adjacent cell is PATH or VISITED, show that in the connecting space
                    if (maze[y][x].type == PATH || (y + 1 < MAZE_HEIGHT && maze[y + 1][x].type == PATH)) {
                        wall_char = 'p';
                    } else if (maze[y][x].type == VISITED || (y + 1 < MAZE_HEIGHT && maze[y + 1][x].type == VISITED)) {
                        wall_char = 'v';
                    }

                    display_buffer[cell_display_y + 1][cell_display_x] = wall_char;
                }

                // 4. Draw corner intersection
                if (x < MAZE_WIDTH - 1 && cell_display_x + 1 < DISPLAY_WIDTH) {
                    // Corner shows wall if either adjacent wall exists
                    if (maze[y][x].right_wall || maze[y][x].bottom_wall ||
                        (x + 1 < MAZE_WIDTH && maze[y][x + 1].bottom_wall) ||
                        (y + 1 < MAZE_HEIGHT && maze[y + 1][x].right_wall)) {
                        display_buffer[cell_display_y + 1][cell_display_x + 1] = '#';
                    } else {
                        // Open corner - inherit from surrounding cells
                        char corner_char = ' ';

                        // Check all four adjacent cells for PATH/VISITED status
                        int has_path = 0, has_visited = 0;

                        if (maze[y][x].type == PATH ||
                            (x + 1 < MAZE_WIDTH && maze[y][x + 1].type == PATH) ||
                            (y + 1 < MAZE_HEIGHT && maze[y + 1][x].type == PATH) ||
                            (x + 1 < MAZE_WIDTH && y + 1 < MAZE_HEIGHT && maze[y + 1][x + 1].type == PATH)) {
                            has_path = 1;
                        }

                        if (maze[y][x].type == VISITED ||
                            (x + 1 < MAZE_WIDTH && maze[y][x + 1].type == VISITED) ||
                            (y + 1 < MAZE_HEIGHT && maze[y + 1][x].type == VISITED) ||
                            (x + 1 < MAZE_WIDTH && y + 1 < MAZE_HEIGHT && maze[y + 1][x + 1].type == VISITED)) {
                            has_visited = 1;
                        }

                        if (has_path) {
                            corner_char = 'p';
                        } else if (has_visited) {
                            corner_char = 'v';
                        }

                        display_buffer[cell_display_y + 1][cell_display_x + 1] = corner_char;
                    }
                }
            }
        }
    }

    // Output the display buffer
    // In BLOCKS mode, use background colors instead of characters
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            char ch = display_buffer[y][x];
            uint8_t color;

            if (ch == '#') {
                // Wall - use white background
                color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_WHITE);
                terminal_putentryat(' ', color, x, y);
            } else if (ch == 'S' || ch == 'E') {
                // Start/End - use special colors with text
                if (ch == 'S') {
                    color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);
                } else {
                    color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_RED);
                }
                terminal_putentryat(ch, color, x, y);
            } else if (ch == 'p') {
                // Path - use bright cyan background
                color = vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_CYAN);
                terminal_putentryat(' ', color, x, y);
            } else if (ch == 'v') {
                // Visited - use brown background
                color = vga_entry_color(VGA_COLOR_BROWN, VGA_COLOR_BROWN);
                terminal_putentryat(' ', color, x, y);
            } else {
                // Corridor/space - use black background
                color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                terminal_putentryat(' ', color, x, y);
            }
        }
    }
}

void generate_maze() {
    maze_init();

    // Reseed for variety on each generation
    seed_rng(timer_ticks());

    // Recursive backtracker (DFS) algorithm
    // Start by marking all cells as walls initially
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            maze[y][x].type = CORRIDOR;
            maze[y][x].right_wall = 1;
            maze[y][x].bottom_wall = 1;
        }
    }

    // Simple iterative backtracker using a stack
    // Stack to track path (max size is width * height)
    int stack_x[MAZE_WIDTH * MAZE_HEIGHT];
    int stack_y[MAZE_WIDTH * MAZE_HEIGHT];
    int stack_size = 0;

    // Visited array
    int visited[MAZE_HEIGHT][MAZE_WIDTH];
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            visited[y][x] = 0;
        }
    }

    // Start at (0, 0)
    int current_x = 0;
    int current_y = 0;
    visited[current_y][current_x] = 1;
    stack_x[stack_size] = current_x;
    stack_y[stack_size] = current_y;
    stack_size++;

    while (stack_size > 0) {
        current_x = stack_x[stack_size - 1];
        current_y = stack_y[stack_size - 1];

        // Find unvisited neighbors
        int neighbors_x[4];
        int neighbors_y[4];
        int neighbors_dir[4]; // 0=up, 1=right, 2=down, 3=left
        int num_neighbors = 0;

        // up
        if (current_y > 0 && !visited[current_y - 1][current_x]) {
            neighbors_x[num_neighbors] = current_x;
            neighbors_y[num_neighbors] = current_y - 1;
            neighbors_dir[num_neighbors] = 0;
            num_neighbors++;
        }
        // down
        if (current_y < MAZE_HEIGHT - 1 && !visited[current_y + 1][current_x]) {
            neighbors_x[num_neighbors] = current_x;
            neighbors_y[num_neighbors] = current_y + 1;
            neighbors_dir[num_neighbors] = 2;
            num_neighbors++;
        }
        // right
        if (current_x < MAZE_WIDTH - 1 && !visited[current_y][current_x + 1]) {
            neighbors_x[num_neighbors] = current_x + 1;
            neighbors_y[num_neighbors] = current_y;
            neighbors_dir[num_neighbors] = 1;
            num_neighbors++;
        }

        // left
        if (current_x > 0 && !visited[current_y][current_x - 1]) {
            neighbors_x[num_neighbors] = current_x - 1;
            neighbors_y[num_neighbors] = current_y;
            neighbors_dir[num_neighbors] = 3;
            num_neighbors++;
        }

        if (num_neighbors > 0) {
            int chosen = random_range(num_neighbors);
            int next_x = neighbors_x[chosen];
            int next_y = neighbors_y[chosen];
            int direction = neighbors_dir[chosen];

            // Remove wall between current and chosen cell
            if (direction == 0) { // up - remove bottom wall of neighbor
                maze[next_y][next_x].bottom_wall = 0;
            } else if (direction == 1) { // right - remove right wall of current
                maze[current_y][current_x].right_wall = 0;
            } else if (direction == 2) { // down - remove bottom wall of current
                maze[current_y][current_x].bottom_wall = 0;
            } else if (direction == 3) { // left - remove right wall of neighbor
                maze[next_y][next_x].right_wall = 0;
            }

            // Mark as visited and push to stack
            visited[next_y][next_x] = 1;
            stack_x[stack_size] = next_x;
            stack_y[stack_size] = next_y;
            stack_size++;
        } else {
            // Backtrack - pop from stack
            stack_size--;
        }
    }

    // Set start and end points
    maze[0][0].type = SOURCE;
    maze[MAZE_HEIGHT - 1][MAZE_WIDTH - 1].type = SINK;

    /*
    // Debug: print the coordinates being set
    int end_x = MAZE_WIDTH - 1;
    int end_y = MAZE_HEIGHT - 1;
    printf("Start set at (0,0), End set at (%d,%d)\n", end_x, end_y);
    */

    draw_maze();
}

int can_move_to(int from_x, int from_y, int to_x, int to_y) {
    // Check bounds
    if (to_x < 0 || to_x >= MAZE_WIDTH || to_y < 0 || to_y >= MAZE_HEIGHT) {
        return 0;
    }

    // Check if there's a wall blocking the path
    if (to_x == from_x + 1 && to_y == from_y) {
        // Moving right - check if current cell has right wall
        return !maze[from_y][from_x].right_wall;
    } else if (to_x == from_x - 1 && to_y == from_y) {
        // Moving left - check if target cell has right wall
        return !maze[to_y][to_x].right_wall;
    } else if (to_y == from_y + 1 && to_x == from_x) {
        // Moving down - check if current cell has bottom wall
        return !maze[from_y][from_x].bottom_wall;
    } else if (to_y == from_y - 1 && to_x == from_x) {
        // Moving up - check if target cell has bottom wall
        return !maze[to_y][to_x].bottom_wall;
    }

    return 0; // Not adjacent or invalid move
}

int dfs_solve(int x, int y, int target_x, int target_y) {
    // Mark current cell as visited
    if (maze[y][x].type == CORRIDOR) {
        maze[y][x].type = VISITED;
    }

    // Check if we reached the target
    if (x == target_x && y == target_y) {
        maze[y][x].type = SINK; // Restore sink marker
        return 1; // Found path
    }

    // Try all four directions
    int directions[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}}; // down, right, up, left

    for (int i = 0; i < 4; i++) {
        int next_x = x + directions[i][0];
        int next_y = y + directions[i][1];

        // Check if we can move to this cell and haven't visited it
        if (can_move_to(x, y, next_x, next_y) &&
                (maze[next_y][next_x].type == CORRIDOR || maze[next_y][next_x].type == SINK)) {

            if (dfs_solve(next_x, next_y, target_x, target_y)) {
                // Mark this cell as part of the solution path
                if (maze[y][x].type != SOURCE) {
                    maze[y][x].type = PATH;
                }
                return 1;
            }
        }
    }

    return 0; // No path found from this cell
}

void display_maze() {
    draw_maze();
}

void solve_maze() {
    // Find start and end positions
    int start_x = -1, start_y = -1;
    int end_x = -1, end_y = -1;

    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (maze[y][x].type == SOURCE) {
                start_x = x;
                start_y = y;
            } else if (maze[y][x].type == SINK) {
                end_x = x;
                end_y = y;
            }
        }
    }

    if (start_x == -1 || end_x == -1) {
        printf("Error: Could not find start or end position\n");
        return;
    }

    // Reset any previous solve attempts
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (maze[y][x].type == VISITED || maze[y][x].type == PATH) {
                maze[y][x].type = CORRIDOR;
            }
        }
    }

    // Restore start and end markers
    maze[start_y][start_x].type = SOURCE;
    maze[end_y][end_x].type = SINK;

    if (dfs_solve(start_x, start_y, end_x, end_y)) {
        // Make sure start position is marked correctly
        maze[start_y][start_x].type = SOURCE;
    } else {
        printf("No solution found!\n");
    }

    draw_maze();
}
