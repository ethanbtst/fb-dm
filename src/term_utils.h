#ifndef TERM_UTILS_H
#define TERM_UTILS_H

#define KEY_BACKSPACE 127


typedef struct {
    int col;
    int row;
} cursor;

// Moves the cursor to a given location
void move_cur(cursor *cur, int row, int col);


// Reads a line from the terminal into a buffer
void readline(cursor *cur, char* buff);

#define move(row, col) move_cur(cur, row, col);

#endif