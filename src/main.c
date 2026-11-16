#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <grp.h>
#include <pwd.h>
#include <sys/ioctl.h>

#include <ncurses.h>
#include <term.h>
#include <security/pam_appl.h>

#include "image.h"
#include "pam_login.h"

void readline(char* buff, bool show){
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

void start_shell(struct passwd* pw){
    char *shell = pw->pw_shell;
    char *base = strrchr(shell, '/');
    base = (base != NULL) ? base + 1 : shell;
    char argv0[256];
    snprintf(argv0, sizeof(argv0), "-%s", base);

    if (chdir(pw->pw_dir) != 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }
    // execlp(, argv0, (char *)NULL);
    execl(pw->pw_shell, pw->pw_shell, "--login", (char *)NULL);
    perror("execl");
    exit(EXIT_FAILURE);
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

    buffer buffer = {0};
    if (!create_buffer(&buffer, "/dev/fb0")) 
        return 1;

    image small_img, large_img;
    if (!load_ppm(&small_img, argv[1]) || !load_ppm(&large_img, argv[2])){
        cleanup((image *[]){&small_img, &large_img}, 2, &buffer);
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

    // Display images
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

    // Text input
    char uname[255] = {0};
    char passwd[255] = {0};

    move((small_img.h + large_img.h) / font_height, 0);
    printw("%s login: ", hostname);
    readline(uname, true);

    printw("password: ");
    readline(passwd, false);
    printw("\n");
    
    // Auth
    pam_handle_t *pamh = NULL;
    if (check_user(uname, passwd, &pamh)){
        struct passwd *pw = getpwnam(uname);
    
        if (!pw) {
            perror("getpwnam");
            return 1;
        }

        if (pam_open_session(pamh, 0) != PAM_SUCCESS) {
            fprintf(stderr, "pam_open_session failed\n");
            pam_setcred(pamh, PAM_DELETE_CRED);
            pam_end(pamh, PAM_SESSION_ERR);
            return 1;
        }
        
        if (initgroups(pw->pw_name, pw->pw_gid) != 0 ||
            setgid(pw->pw_gid) != 0 ||
            setuid(pw->pw_uid) != 0) {
                perror("privilege drop failed");
                pam_close_session(pamh, PAM_SILENT);
                pam_setcred(pamh, PAM_DELETE_CRED);
                pam_end(pamh, PAM_SYSTEM_ERR);
                return 1;
        }
            
        if (!setup_env(pamh, pw)) {
            perror("environment setup failed");
            pam_close_session(pamh, PAM_SILENT);
            pam_setcred(pamh, PAM_DELETE_CRED);
            pam_end(pamh, PAM_SYSTEM_ERR);
            return 1;
        }

        cleanup((image *[]){&small_img, &large_img}, 2, &buffer);
        start_shell(pw);
        
        pam_close_session(pamh, PAM_SILENT);
        pam_setcred(pamh, PAM_DELETE_CRED);
        pam_end(pamh, PAM_SUCCESS);
    } else {
        printw("Nope");
    }
    return 0;
}
