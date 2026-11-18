#include "info.h"
#include "sessions.h"
#include "term_utils.h"
#include <stdio.h>
#include <time.h>

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

void session_info(session *sessions, int session_cur, int count)
{
	if (sessions == NULL || session_cur < 0 || session_cur > count)
		return;

	printf("%s: < %s >", sessions[session_cur].type,
		   sessions[session_cur].name);
}