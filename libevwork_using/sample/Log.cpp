#include "Log.h"
#include <stdio.h>
#include "Types.h"

extern struct g_args_s g_args;

static int logfd = STDOUT_FILENO;
static const char *log_level_map[] = {"ERROR", "INFO", "DEBUG"};
static char	msgbuf[MSG_BUFF_SIZE];
static char	logbuf[LOG_BUFF_SIZE];

void loginit()
{
	char	path[64] = "\0";
	mkdir(g_args.logdir.c_str(), 0755);
	snprintf(path, 64, "%s/log", g_args.logdir.c_str());
	logfd = open(path, O_RDWR | O_CREAT | O_APPEND, 0666);
}

void logDinit()
{
	close(logfd);
}

void dolog(int level, const char *fmt, ...)
{
	if (unlikely(logfd == STDOUT_FILENO)) {
		return;
	}

	time_t  iTime = time(NULL);
	struct tm* pTM = localtime(&iTime);
	va_list     ap; 

	memset(logbuf, 0, LOG_BUFF_SIZE);
	memset(msgbuf, 0, MSG_BUFF_SIZE);

	va_start(ap, fmt);
	vsnprintf(msgbuf, MSG_BUFF_SIZE, fmt, ap);
	va_end(ap);

	snprintf(logbuf, LOG_BUFF_SIZE, "[%04d-%02d-%02d %02d:%02d:%02d %s] %s\n", pTM->tm_year + 1900, pTM->tm_mon + 1,
			pTM->tm_mday, pTM->tm_hour, pTM->tm_min, pTM->tm_sec, log_level_map[level], msgbuf);
	write(logfd, logbuf, strlen(logbuf));
	//fsync(logfd);
}
