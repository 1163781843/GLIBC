#ifndef __JEMALLOC_H__
#define __JEMALLOC_H__

#ifdef GRANDSTREAM_NETWORKS
static inline struct tm *jelocaltime(const time_t *srctime, struct tm *dsttime)
{
	long int n32_Pass4year,n32_hpery;

	// 每个月的天数  非闰年
	static const char Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	// 一年的小时数
	static const int ONE_YEAR_HOURS = 8760; // 365 * 24 (非闰年)

	//计算时差8*60*60 固定北京时间
	time_t time = *srctime;
	time = time + 28800;
	dsttime->tm_isdst = 0;
	if(time < 0) {
		time = 0;
	}

	//取秒时间
	dsttime->tm_sec = (int)(time % 60);
	time /= 60;

	//取分钟时间
	dsttime->tm_min = (int)(time % 60);
	time /= 60;

	//计算星期
	dsttime->tm_wday = (time/24 + 4) % 7;

	//取过去多少个四年，每四年有 1461*24 小时
	n32_Pass4year = ((unsigned int)time / (1461L * 24L));

	//计算年份
	dsttime->tm_year = (n32_Pass4year << 2) + 70;

	//四年中剩下的小时数
	time %= 1461L * 24L;

	//计算在这一年的天数
	dsttime->tm_yday = (time / 24) % 365;

	//校正闰年影响的年份，计算一年中剩下的小时数
	for (;;) {
		//一年的小时数
		n32_hpery = ONE_YEAR_HOURS;

		//判断闰年
		if ((dsttime->tm_year & 3) == 0) {
			//是闰年，一年则多24小时，即一天
			n32_hpery += 24;
		}

		if (time < n32_hpery) {
			break;
		}

		dsttime->tm_year++;
		time -= n32_hpery;
	}

	//小时数
	dsttime->tm_hour = (int)(time % 24);

	//一年中剩下的天数
	time /= 24;

	//假定为闰年
	time++;

	//校正润年的误差，计算月份，日期
	if ((dsttime->tm_year & 3) == 0) {
		if (time > 60) {
			time--;
		} else {
			if (time == 60) {
				dsttime->tm_mon = 1;
				dsttime->tm_mday = 29;
				return dsttime;
			}
		}
	}

	//计算月日
	for (dsttime->tm_mon = 0;Days[dsttime->tm_mon] < time;dsttime->tm_mon++) {
		time -= Days[dsttime->tm_mon];
	}

	dsttime->tm_mday = (int)(time);

	return dsttime;
}

static inline void jelogger(int level, const char *file, int line, const char *format, ...)
{
	char buffer[65535] = {0};
	va_list ap;
	int size = 0;
	char *filename = NULL;
	struct timeval cur_tv;
	time_t cur_time;
	struct tm cur_format_time;

	if (file) {
		filename = strrchr(file, '/');
	}

	cur_time = time(NULL);

	jelocaltime(&cur_time, &cur_format_time);

	gettimeofday(&cur_tv, NULL);

	size = snprintf(buffer, sizeof(buffer) - 1, "[%04d:%02d:%02d %02d:%02d:%02d:%06ld] [%ld] [%s:%d] -- ",
		cur_format_time.tm_year + 1900,
		cur_format_time.tm_mon + 1,
		cur_format_time.tm_mday,
		cur_format_time.tm_hour,
		cur_format_time.tm_min,
		cur_format_time.tm_sec,
		cur_tv.tv_usec,
		syscall(SYS_gettid), filename ? ++filename : file, line);

	va_start(ap, format);
	vsnprintf(buffer + size, sizeof(buffer) - size - 1, format, ap);
	va_end(ap);

	fprintf(stderr, "%s\n", buffer);
}

#define jelog(level, ...) do {								\
	if (level) {											\
		jelogger(level, __FILE__, __LINE__, __VA_ARGS__);	\
	}														\
} while (0)
#endif

#endif
