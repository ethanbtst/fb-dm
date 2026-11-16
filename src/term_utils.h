#ifndef TERM_UTILS_H
#define TERM_UTILS_H

#define KEY_BACKSPACE 127


typedef struct {
    int row;
    int col;
} cursor;

// Moves the cursor to a given location
void move_cur(cursor *cur, int x, int y);


// Reads a line from the terminal into a buffer
void readline(cursor *cur, char* buff);

#define move(x, y) move_cur(cur, x, y);

#endif