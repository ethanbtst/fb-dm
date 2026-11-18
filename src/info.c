#include "info.h"
#include "sessions.h"
#include "term_utils.h"
#include <stdio.h>
#include <time.h>
#include <glob.h>

char info_buff[255];

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

void update_brightness(int change)
{
	glob_t backlight_files = { 0 };

	int glob_result = glob("/sys/class/backlight/*/brightness", GLOB_ERR, NULL,
						   &backlight_files);
	if (glob_result == GLOB_NOMATCH)
		return;

	glob_result = glob("/sys/class/backlight/*/brightnessmax",
						   GLOB_ERR | GLOB_APPEND, NULL, &backlight_files);
	if (glob_result == GLOB_NOMATCH)
		return;

	FILE *fd = fopen(backlight_files.gl_pathv[0], "r");
	int brightness = 0;
	fscanf(fd, "%d", &brightness);
	fclose(fd);

	fd = fopen(backlight_files.gl_pathv[1], "r");
	int max_brightness = 0;
	fscanf(fd, "%d", &max_brightness);
	fclose(fd);

	globfree(&backlight_files);

	change = (5/100) * max_brightness * change;
	printf("%d", change);
}