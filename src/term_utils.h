#ifndef TERM_UTILS_H
#define TERM_UTILS_H
#include <termios.h>

typedef struct {
    int col;
    int row;
} cursor;

void setup_term();

// Moves the cursor to a given location
void move_cur(cursor *cur, int row, int col);

// Reads a line from the terminal into a buffer
void readline(cursor *cur, char* buff);

#define move(row, col)  move_cur(cur, row, col);
#define echo()          {current_term.c_lflag &= ECHO; tcsetattr(STDIN_FILENO, TCSANOW, &current_term);}
#define no_echo()       {current_term.c_lflag &= ~ECHO; tcsetattr(STDIN_FILENO, TCSANOW, &current_term);}
#define KEY_BACKSPACE   127

extern cursor *cur;
extern struct termios current_term;
extern struct termios saved_term;
#endif