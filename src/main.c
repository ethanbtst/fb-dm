#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <ctype.h>

#include <grp.h>
#include <pwd.h>
#include <sys/ioctl.h>

#include <security/pam_appl.h>
#include <termios.h>

#include "image.h"
#include "pam_login.h"
#include "term_utils.h"

// Config
char opt_small_pic[255];
char opt_big_pic[255];
bool opt_hostname;
bool opt_kernel;
char opt_top_text[255];
char opt_date[255];
char opt_time[255];

static char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        text++;

    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
        *--end = '\0';

    return text;
}

static bool parse_line(char *line, char **option, char **value)
{
    char *separator = strchr(line, '=');

    if (separator == NULL)
        return false;

    *separator = '\0';
    *option = trim(line);
    *value = trim(separator + 1);

    if (**option == '\0' || **option == '#')
        return false;

    if ((*value)[0] == '\0')
        return false;

    return true;
}

void set_distro_logo()
{
    FILE *fd = fopen("/etc/os-release", "r");
    
    char line[255];
    char *option;
    char *value;
    
    while(fgets(line, sizeof(line), fd)){

        if(!parse_line(line, &option, &value))
            continue;

        if (strcasecmp(option, "id") == 0)
            snprintf(opt_big_pic, sizeof(opt_big_pic),
                     "/opt/fb-dm/img/distro_logos/%s.ppm", value);
  
    }
    fclose(fd);
}

void parse_config()
{
    FILE *fd = fopen("/opt/fb-dm/config.ini", "r");
    
    if (!fd)  
        return;

    char line[255];
    char *option;
    char *value;
    
    while(fgets(line, sizeof(line), fd)){

        if(!parse_line(line, &option, &value))
            continue;

        if (strcmp(option, "small_pic") == 0)
            strcpy(opt_small_pic, value);

        if (strcmp(option, "big_pic") == 0)
            strcpy(opt_big_pic, value);

        if (strcmp(option, "top_text") == 0)
            strcpy(opt_top_text, value);

        if (strcmp(option, "hostname") == 0)
            if (strcmp(value, "false") == 0)
                opt_hostname = false;

        if (strcmp(option, "kernel") == 0)
            if (strcmp(value, "false") == 0)
                opt_kernel = false;

        if (strcmp(option, "date") == 0)
            strcpy(opt_date, value);

        if (strcmp(option, "time") == 0)
            strcpy(opt_time, value);
    }

    fclose(fd);
}

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

void start_shell(struct passwd *pw){
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

int main()
{
    // default config
    strcpy(opt_small_pic, "/opt/fb-dm/img/logo_linux_clut224.ppm");
    strcpy(opt_big_pic, "distro");
    strcpy(opt_top_text, "Welcome!");
    opt_hostname = true;
    opt_kernel = true;
    strcpy(opt_date, "%A %B %d, %Y");
    strcpy(opt_time, "%R");

    setup_term();
    parse_config();
    if (strcmp(opt_big_pic, "distro") == 0)
        set_distro_logo();
    
    setlocale(LC_ALL, "");

    buffer *buf = &(buffer){ .fbp = NULL };
    if (!create_buffer(buf, "/dev/fb0")) 
        return 1;

    image small_img, large_img;
    if (!load_ppm(&small_img, opt_small_pic) || !load_ppm(&large_img, opt_big_pic)){
        cleanup((image *[]){&small_img, &large_img}, 2,  &saved_term, buf);
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
        draw_image(buf, &small_img, i *small_img.w, 0);
    draw_image(buf, &large_img, 0, small_img.h);
   
    // Info
    char hostname[255];
    char kernel[255];
    char date[255];
    char time_s[255];

    printf("%s", opt_top_text); move(cur->row + 2, right);

    FILE *fptr = fopen("/etc/hostname", "rb");
    fscanf(fptr, "%254s", hostname);
    fclose(fptr);
    if (opt_hostname) {
        printf("[%s]", hostname);
        move(cur->row + 2, right);
    }

    fptr = fopen("/proc/version", "rb");
    fscanf(fptr, "%*s %*s %254s", kernel);
    fclose(fptr);
    if (opt_kernel) {
        printf("Kernel: %s", kernel);
        move(cur->row + 2, right);
    }

    time_t t;
    struct tm *tmp;
    time(&t); tmp = localtime(&t);

    strftime(date, sizeof(date), opt_date, tmp);
    printf("Date: %s", date); move(cur->row + 2, right); 

    strftime(time_s, sizeof(time_s), opt_time, tmp);
    printf("Time: %s", time_s); move(cur->row + 2, right); 

    printf("Other: < %s >", "Shell"); move(cur->row + 2, right); 

    // Text input
    char uname[255] = {0};
    char passwd[255] = {0};

    move(((small_img.h + large_img.h) / font_height) + 1, 0);
    printf("%s login: ", hostname);
    readline(cur, uname);

    printf("password: ");
    no_echo();
    readline(cur, passwd);
    printf("\n");
    echo();

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

        cleanup((image *[]){&small_img, &large_img}, 2,  &saved_term, buf);
        start_shell(pw);
        
        pam_close_session(pamh, PAM_SILENT);
        pam_setcred(pamh, PAM_DELETE_CRED);
        pam_end(pamh, PAM_SUCCESS);
    } else {
        printf("Login Failed\n");
    }
    return 0;
}
