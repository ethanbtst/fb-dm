#include "info.h"
#include "sessions.h"
#include "term_utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <glob.h>

char info_buff[255];
int brightness_delta;
int max_brightness;

void file_info(char *filename, char *in_fmt, char *out_fmt)
{
	FILE *fd = fopen(filename, "r");
	if (fd == NULL)
		return;
	fscanf(fd, in_fmt, info_buff);
	fclose(fd);
	int col = get_col();
	printf(out_fmt, info_buff);
	move(cur->row + 2, col);
}

void time_info(char *name, char *fmt)
{
	time_t t;
	struct tm *tmp;
	time(&t);
	tmp = localtime(&t);

	int col = get_col();
	strftime(info_buff, sizeof(info_buff), fmt, tmp);
	printf("%s: %s", name, info_buff);
	move(cur->row + 2, col);
}

void battery_info()
{
	glob_t bat_file = { 0 };
	int glob_result =
		glob("/sys/class/power_supply/*/capacity", GLOB_ERR, NULL, &bat_file);

	if (glob_result == GLOB_NOMATCH)
		return;

	int charge = 0;
	FILE *fd = fopen(bat_file.gl_pathv[0], "r");
	fscanf(fd, "%d", &charge);
	fclose(fd);
	globfree(&bat_file);

	int col = get_col();
	printf("Bat: %d%%", charge);
	move(cur->row + 2, col);
}

void session_info(session *sessions, int session_cur, int count)
{
	if (sessions == NULL || session_cur < 0 || session_cur > count)
		return;

	printf("%s: < %s >", sessions[session_cur].type,
		   sessions[session_cur].name);
}

static char brightness_path[4096];

static int read_brightness(const char *path, int *value)
{
	FILE *fd = fopen(path, "r");
	if (fd == NULL)
		return 0;

	int result = fscanf(fd, "%d", value) == 1;
	fclose(fd);
	return result;
}

void set_brightness_values(void)
{
	glob_t backlight_file = { 0 };
	max_brightness = 0;
	brightness_delta = 0;
	brightness_path[0] = '\0';

	int glob_result =
		glob("/sys/class/backlight/*/brightness", GLOB_ERR, NULL,
			 &backlight_file);
	if (glob_result != 0 || backlight_file.gl_pathc == 0) {
		globfree(&backlight_file);
		return;
	}

	snprintf(brightness_path, sizeof(brightness_path), "%s",
			 backlight_file.gl_pathv[0]);

	char *slash = strrchr(brightness_path, '/');
	if (slash == NULL) {
		brightness_path[0] = '\0';
		globfree(&backlight_file);
		return;
	}

	char max_path[4096];
	snprintf(max_path, sizeof(max_path), "%.*s/max_brightness",
			 (int)(slash - brightness_path), brightness_path);
	globfree(&backlight_file);

	if (!read_brightness(max_path, &max_brightness) || max_brightness <= 0)
		return;

	brightness_delta = (5 * max_brightness) / 100;
	if (brightness_delta < 1)
		brightness_delta = 1;
}

void update_brightness(int change)
{
	if (change == 0)
		return;
	if (brightness_path[0] == '\0')
		set_brightness_values();
	if (brightness_path[0] == '\0' || max_brightness <= 0)
		return;

	int brightness;
	if (!read_brightness(brightness_path, &brightness))
		return;

	brightness += change * brightness_delta;
	if (brightness < 0)
		brightness = 0;
	else if (brightness > max_brightness)
		brightness = max_brightness;

	FILE *fd = fopen(brightness_path, "w");
	if (fd == NULL)
		return;

	fprintf(fd, "%d\n", brightness);
	fclose(fd);
}