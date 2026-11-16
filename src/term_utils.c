#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "term_utils.h"

void move_cur(cursor *cur, int x, int y)
{
    cur->col = x;
    cur->row = y; 
    printf("\033[%d;%dH", x, y);
}

void readline(cursor *cur, char* buff)
{
    int ch;
        while ((ch = getchar()) != '\n'){
        if (ch >= 32 && ch <= 126){
            strcat(buff, (char *)&ch);
        } else if (ch == '\b' || ch == KEY_BACKSPACE){
            if (strnlen(buff, sizeof(buff)) == 0) 
                continue;
            buff[strlen(buff) - 1] = '\0';
        }
    }
    buff[strlen(buff)] = '\0';
    cur->row = cur->row + 1;
    cur->col = cur->col + strlen(buff);
}
