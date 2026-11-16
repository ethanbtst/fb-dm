#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <grp.h>
#include <pwd.h>
#include <sys/ioctl.h>

#include <security/pam_appl.h>
#include <termios.h>

#include "image.h"
#include "pam_login.h"
#include "term_utils.h"



void cleanup(image *images[], size_t image_count, struct termios *term, buffer *buffer){
    for (size_t i = 0; i < image_count; ++i){
        if (images[i] != NULL) {
            free(images[i]->data);
            images[i]->data = NULL;
        }
    }

    destroy_buffer(buffer);
    tcsetattr(STDIN_FILENO, TCSANOW, term);
}

void start_shell(struct passwd* pw){
    char *shell = pw->pw_shell;
    char *base = strrchr(shell, '/');
    char argv0[256];
    
    if (chdir(pw->pw_dir) != 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }
    
    base = (base != NULL) ? base + 1 : shell;
    snprintf(argv0, sizeof(argv0), "-%s", base);
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

    cursor *cur = &(cursor){1,1};
    printf("\033[2J\033[H");
    move(100, 2);
    fflush(stdout);

    struct termios term_normal, term_no_echo;
    tcgetattr(STDIN_FILENO, &term_normal);
    term_no_echo = term_normal;
    term_no_echo.c_lflag &= ~ECHO;

    buffer *buf = &(buffer){ .fbp = NULL };
    if (!create_buffer(buf, "/dev/fb0")) 
        return 1;

    image small_img, large_img;
    if (!load_ppm(&small_img, argv[1]) || !load_ppm(&large_img, argv[2])){
        cleanup((image *[]){&small_img, &large_img}, 2,  &term_normal, buf);
        return 1;
    }

    // Positioning 
    struct winsize w;
    ioctl(STDERR_FILENO, TIOCGWINSZ, &w);
    int font_height = buf->h / w.ws_row;
    int font_width = buf->w / w.ws_col;

    int right = large_img.w/font_width + 2;
    move((small_img.h/font_height) + 2, right);

    // Display images
    for (int i = 0; i < sysconf(_SC_NPROCESSORS_ONLN); ++i) 
        draw_image(buf, &small_img, i * small_img.w, 0);
    draw_image(buf, &large_img, 0, small_img.h);
   
    // Info
    char hostname[255];
    char kernel[255];
    char date[255];
    char time_s[255];

    printf("Welcome!"); move(cur->col + 2, right); 

    FILE *fptr = fopen("/etc/hostname", "rb");
    fscanf(fptr, "%s", hostname); 
    fclose(fptr);
    printf("[%s]", hostname); move(cur->col + 2, right); 

    fptr = fopen("/proc/version", "rb");
    fscanf(fptr, "%*s %*s %s", kernel); 
    fclose(fptr);
    printf("Kernel: %s", kernel); move(cur->col + 2, right); 


    time_t t;
    struct tm* tmp;
    time(&t); tmp = localtime(&t);

    strftime(date, sizeof(date), "%A %B %d, %Y", tmp);
    printf("Date: %s", date); move(cur->col + 2, right); 

    strftime(time_s, sizeof(time_s), "%R", tmp);
    printf("Time: %s", time_s); move(cur->col + 2, right); 

    // Text input
    char uname[255] = {0};
    char passwd[255] = {0};

    tcsetattr(STDIN_FILENO, TCSANOW, &term_normal);
    move((small_img.h + large_img.h) / font_height, 0);
    printf("%s login: ", hostname);
    readline(cur, uname);

    tcsetattr(STDIN_FILENO, TCSANOW, &term_no_echo);
    printf("password: ");
    readline(cur, passwd);
    printf("\n");
    
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

        cleanup((image *[]){&small_img, &large_img}, 2,  &term_normal, buf);
        start_shell(pw);
        
        pam_close_session(pamh, PAM_SILENT);
        pam_setcred(pamh, PAM_DELETE_CRED);
        pam_end(pamh, PAM_SUCCESS);
    } else {
        printf("Nope");
    }
    return 0;
}
