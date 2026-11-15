#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <term.h>
#include "image.h"
#include "pam_login.h"
#include <pwd.h>
#include <grp.h>

void Readline(char* buff, bool show){
    int ch;
        while ((ch = getch()) != '\n'){
        if (ch >= 32 && ch <= 126){
            strcat(buff, (char *)&ch);
            if (show) printw("%c", ch);
            else printw("*");

        } else if (ch == '\b' || ch == 127 || ch == KEY_BACKSPACE){
            if (strnlen(buff, sizeof(buff)) == 0) 
                continue;
            buff[strlen(buff) - 1] = 0;
            move(getcury(stdscr), getcurx(stdscr) - 1);
            printw(" ");
            move(getcury(stdscr), getcurx(stdscr) - 1);
        }
    }
    buff[strlen(buff)] = '\0';
    printw("\n");
}

void cleanup(image *images[], size_t image_count, buffer *buffer){
    for (size_t i = 0; i < image_count; ++i){
        if (images[i] != NULL) {
            free(images[i]->data);
            images[i]->data = NULL;
        }
    }

    destroy_buffer(buffer);
    int x = getcurx(stdscr);
    int y = getcury(stdscr);
    endwin();
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <small.ppm> <large.ppm>\n", argv[0]);
        return 1;
    }

    // Leave text after program exit
    setupterm(NULL, STDOUT_FILENO, NULL);
    enter_ca_mode = NULL;
    exit_ca_mode = NULL;
    initscr();
    curs_set(2);
    cbreak();
    noecho();
    int y, x;
    getyx(stdscr, y, x);

    buffer buffer = {0};
    if (!create_buffer(&buffer, "/dev/fb0")) 
        return 1;

    image small_img, large_img;
    if (!load_ppm(&small_img, argv[1]) || !load_ppm(&large_img, argv[2])){
        free(small_img.data);
        free(large_img.data);
        destroy_buffer(&buffer);
        return 1;
    }

    // Positioning 
    struct winsize w;
    ioctl(STDERR_FILENO, TIOCGWINSZ, &w);
    int font_height = buffer.h / w.ws_row;
    int font_width = buffer.w / w.ws_col;

    int right = large_img.w/font_width + 2;
    move((small_img.h/font_height) + 2, right);
    refresh();

    // Images
    for (int i = 0; i < sysconf(_SC_NPROCESSORS_ONLN); ++i) 
        draw_image(&buffer, &small_img, i * small_img.w, 0);
    draw_image(&buffer, &large_img, 0, small_img.h);
    
    // Info
    char hostname[255];
    char kernel[255];
    char date[255];
    char time_s[255];

    printw("Welcome!"); move(getcury(stdscr) + 2, right); 

    FILE *fptr = fopen("/etc/hostname", "rb");
    fscanf(fptr, "%s", hostname); 
    fclose(fptr);
    printw("[%s]", hostname); move(getcury(stdscr) + 2, right); 

    fptr = fopen("/proc/version", "rb");
    fscanf(fptr, "%*s %*s %s", kernel); 
    fclose(fptr);
    printw("Kernel: %s", kernel); move(getcury(stdscr) + 2, right); 


    time_t t;
    struct tm* tmp;
    time(&t); tmp = localtime(&t);

    strftime(date, sizeof(date), "%A %B %d, %Y", tmp);
    printw("Date: %s", date); move(getcury(stdscr) + 2, right); 

    strftime(time_s, sizeof(time_s), "%R", tmp);
    printw("Time: %s", time_s); move(getcury(stdscr) + 2, right); 

    // Login-ish area
    char uname[255] = {0};
    char passwd[255] = {0};
    
    move((small_img.h + large_img.h) / font_height, 0);
    printw("%s login: ", hostname);
    Readline(uname, true);

    printw("password: ");
    Readline(passwd, false);
    printw("\n");
    
    if (check_user(uname, passwd)){
        struct passwd *pw = getpwnam(uname);
        if (pw == NULL) 
            return 1;

        if (initgroups(pw->pw_name, pw->pw_gid) != 0)
            return 1;

        if (setgid(pw->pw_gid) != 0) 
            return 1;
        
        if (setuid(pw->pw_uid) != 0) 
            return 1;
        
        cleanup((image *[]){&small_img, &large_img}, 2, &buffer);

        char *shell = pw->pw_shell;

        char *base = strrchr(shell, '/');
        base = (base != NULL) ? base + 1 : shell;

        char argv0[256];
        snprintf(argv0, sizeof(argv0), "-%s", base);

        // execl(pw->pw_shell, "-bash", "--login", (char *)NULL);
        execl(pw->pw_shell, base, (char *)NULL);
        perror("execl");
        return 1;
    } else {
        printw("Nope");
    }
    return 0;
}
