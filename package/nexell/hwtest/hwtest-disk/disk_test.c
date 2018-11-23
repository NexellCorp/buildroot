/*
 * Copyright (C) 2018  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE	/* for O_DIRECT */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <linux/sched.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>

#define	DISK_SIGNATURE		0xD150D150

#define	KBYTE			(1024)
#define	MBYTE			(1024 * KBYTE)
#define	KB(v)			(v * KBYTE)
#define	MB(v)			(KB(v) * KBYTE)

#define	BUFFER_DEF_SIZE		MB(1)
#define	BUFFER_MIN_SIZE		(4)
#define	BUFFER_MAX_SIZE		MB(50)
#define	FILE_DEF_SIZE		BUFFER_MAX_SIZE
#define	FILE_MIN_SIZE		MB(1)
#define	FILE_MAX_SIZE		MB(100)
#define	SPARE_SIZE		MB(5)

#define	FILE_PREFIX		"test"
#define	DISK_PATH		"./"
#define	DISK_COUNT		(1)
#define SECTOR_SIZE		KB(1)

#define	FILE_O_SYNC		(1<<0)
#define	FILE_O_DIRECT		(1<<1)

#define	FILE_W_FLAG		(O_RDWR | O_CREAT)
#define	FILE_R_FLAG		(O_RDONLY)

#define	SE(_us)			(_us/1000000)
#define	US(_us)			(_us%1000000)

#define MBS(_l, _u)		((((u64)_l/(u64)_u)*1000000)/(u64)MBYTE)
#define	MBU(_l, _u)		((((u64)_l/(u64)_u)*1000000)%(u64)MBYTE)

#define	RUN_TIME_US(s) { \
	struct timeval tv; \
	u64 t; \
	gettimeofday(&tv, NULL); \
	t = (tv.tv_sec*1000000) + (tv.tv_usec),	s = t; \
	while (s == t) { \
		gettimeofday(&tv, NULL); \
		t = (tv.tv_sec*1000000) + (tv.tv_usec);	\
	} \
	s = t; \
	}

#define	END_TIME_US(s, e) { \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	e = (tv.tv_sec*1000000) + (tv.tv_usec);	\
	e = e - s; \
	}

#define RAND_SIZE(min, max, aln, val) { \
	val = rand() % max; \
	val = ((val+aln-1)/aln)*aln; \
	if (min > val) \
		val = min; \
	}

#define	MSG(exp...)	fprintf(stdout, exp)
#define	ERR(exp...)	fprintf(stderr, exp)

typedef unsigned long long  u64;

static int sched_set_new(pid_t pid, int policy, int priority)
{
	struct sched_param param;
	int maxpri = 0, minpri = 0;
	int ret;

	switch (policy) {
	case SCHED_NORMAL:
	case SCHED_FIFO:
	case SCHED_RR:
	case SCHED_BATCH:
		break;
	default:
		ERR("invalid policy %d (0~3)\n", policy);
		return -EINVAL;
	}

	if (policy == SCHED_NORMAL) {
		/*
		 * #define NICE_TO_PRIO(nice)
		 *(MAX_RT_PRIO + (nice) + 20), MAX_RT_PRIO 100
		 */
		maxpri =  20;
		minpri = -20; /* nice */
	} else {
		maxpri = sched_get_priority_max(policy);
		minpri = sched_get_priority_min(policy);
	}

	if ((priority > maxpri) || (minpri > priority)) {
		ERR("\nFail, invalid priority (%d ~ %d)...\n",
			minpri, maxpri);
		return -EINVAL;
	}

	if (policy == SCHED_NORMAL) {
		param.sched_priority = 0;
		ret = sched_setscheduler(pid, policy, &param);
		if (ret) {
			ERR("Fail, sched_setscheduler (%d)\n\n", ret);
			return ret;
		}
		ret = setpriority(PRIO_PROCESS, pid, priority);
		if (ret)
			ERR("Fail, setpriority (%d)\n\n", ret);
	} else {
		param.sched_priority = priority;
		ret = sched_setscheduler(pid, policy, &param);
		if (ret)
			ERR("Fail, sched_setscheduler (%d)\n\n", ret);
	}

	return ret;
}

static long long disk_disk_avail(const char *disk, long long *ptot,
				 int debug)
{
	long long total, used, free, avail;
	double used_percent;
	struct statfs64 fs;
	struct stat st;

	if (stat(disk, &st)) {
		if (mkdir(disk, 0755)) {
			ERR("Fail, make dir path:%s (%d)\n", disk, errno);
			return 0;
		}
	} else {
		if (!S_ISDIR(st.st_mode)) {
			ERR("Fail, file exist %s (%d)!!!\n", disk, errno);
			return 0;
		}
	}

	if (statfs64(disk, &fs) < 0) {
		ERR("Fail, status fs %s (%d)\n", disk, errno);
		return 0;
	}

	total = (fs.f_bsize * fs.f_blocks);
	free = (fs.f_bsize * fs.f_bavail);
	avail = (fs.f_bsize * fs.f_bavail);
	used = (total - free);
	used_percent = ((double)used/(double)total)*100;

	if (debug) {
		MSG("--------------------------------------\n");
		MSG("PATH       : %s\n", disk);
		MSG("Total size : %16lld\n", total);
		MSG("Free  size : %16lld\n", free);
		MSG("Avail size : %16lld\n", avail);
		MSG("Used  size : %16lld\n", used);
		MSG("Block size : %8d B\n", (int)fs.f_bsize);
		MSG("Use percent: %16f %%\n", used_percent);
		MSG("--------------------------------------\n");
	}

	if (ptot)
		*ptot = total;

	return avail;
}

static int disk_obtain_space(const char *disk, int search, long long request)
{
	char file[256];
	long long avail;
	int  i = 0;

	while (1) {
		/* exist file */
		sprintf(file, "%s/%s.%d.txt", disk, FILE_PREFIX, i);

		if (access(file, F_OK) < 0) {
			if (i > search) {
				ERR("Fail, access %s (%d)\n", file, errno);
				return -EINVAL;
			}
			i++; /* next file */
			continue;
		}

		/* remove to obtain free */
		if (remove(file) < 0) {
			ERR("Fail, remove %s (%d)\n", file, errno);
			return -EINVAL;
		}
		sync();

		/* check free */
		avail = disk_disk_avail(disk, NULL, 0);
		if (avail > request)
			break;
		i++; /* next file */
	}

	return 0;
}

static int file_read_sign(const char *file,
			  long long *pf_length, int *pb_length)
{
	unsigned int data[4] = { 0, };
	long long f_size;
	int fd, b_size;
	int ret;

	fd = open(file, O_RDWR|O_SYNC, 0777);
	if (fd < 0) {
		fd = open(file, O_RDWR, 0777);
		if (fd < 0)
			return -EINVAL;
	}

	ret = read(fd, (void *)data, sizeof(data));
	close(fd);

	if (ret < (int)sizeof(data))
		return -EINVAL;

	if (data[0] != DISK_SIGNATURE) {
		ERR("Fail, Unknown signature 0x%08x (0x%08x)\n",
		     data[0], DISK_SIGNATURE);
		return -EINVAL;
	}

	b_size = data[1];
	f_size = (long long)data[2];

	if (pf_length)
		*pf_length = f_size;

	if (pb_length)
		*pb_length = b_size;

	return 0;
}

static int file_write_sign(const char *file, long long f_length, int b_length)
{
	unsigned int data[4];
	int fd, ret;

	fd = open(file, O_RDWR|O_SYNC, 0777);
	if (fd < 0) {
		ERR("Fail, info open %s (%d)\n", file, errno);
		return -EINVAL;
	}

	data[0] = DISK_SIGNATURE;
	data[1] = b_length;
	data[2] = (f_length) & 0xFFFFFFFF;
	data[3] = (f_length >> 32) & 0xFFFFFFFF;

	ret = write(fd, (void *)&data, sizeof(data));
	close(fd);

	if (ret < (int)sizeof(data))
		return -EINVAL;

	sync();

	return 0;
}

static long long file_write(const char *file, unsigned long f_flags,
			    long long f_length, int b_length, u64 *time,
			    int wo, int verify)
{
	int fd, flags = O_RDWR | O_CREAT;
	int *buf;
	long long w_len, r_len;
	u64 ts = 0, te;
	int count;
	int i, ret;

	if (b_length > BUFFER_MAX_SIZE)
		b_length = BUFFER_MAX_SIZE;

	if (b_length > f_length)
		b_length = f_length;

	/*
	 * O_DIRECT로 열린 파일은 기록하려는 메모리 버퍼, 파일의 오프셋,
	 * 버퍼의 크기가모두 디스크 섹터 크기의 배수로 정렬되어 있어야 한다.
	 * 따라서 메모리는 posix_memalign을 사용해서 섹터 크기로 정렬되도록
	 * 해야 한다.
	 */
	ret = posix_memalign((void *)&buf, SECTOR_SIZE, b_length);
	if (ret) {
		ERR("Fail: allocate memory buffer %d (%d)\n", b_length, ret);
		return -ENOMEM;
	}

	/* fill buffer */
	for (i = 0; i < b_length/4; i++)
		buf[i] = i;

	/* wait for "start of" clock tick */
	if (f_flags & FILE_O_SYNC)
		sync();

	flags = FILE_W_FLAG | (f_flags & FILE_O_DIRECT ? O_DIRECT : 0);

	fd = open(file, flags, 0777);
	if (fd < 0) {
		flags = FILE_W_FLAG | (f_flags & FILE_O_SYNC ? O_SYNC : 0);

		fd = open(file, flags, 0777);
		if (fd < 0) {
			flags = FILE_W_FLAG;

			fd = open(file, flags, 0777);
			if (fd < 0) {
				ERR("Fail, write open %s (%d)\n", file, errno);
				free(buf);
				return -EINVAL;
			}
		}
	}

	count = b_length, w_len = 0;

	if (time)
		RUN_TIME_US(ts);

	while (count > 0) {
		ret = write(fd, buf, count);
		if (ret < 0) {
			ERR("Fail, wrote %lld (%d)\n", w_len, errno);
			break;
		}

		w_len += ret;
		count = f_length - w_len;

		if (count > b_length)
			count = b_length;
	}

	/* End */
	if (f_flags & FILE_O_SYNC)
		sync();

	if (time) {
		END_TIME_US(ts, te);
		*time = te;
	}

	close(fd);

	if (w_len != f_length) {
		free(buf);
		return -EINVAL;
	}

	/* set test file info */
	if (file_write_sign(file, f_length, b_length) < 0)
		return -EINVAL;

	if (wo)
		return f_length;

	/* verify open */
	flags = O_RDONLY | (flags & ~(FILE_W_FLAG));

	fd = open(file, flags, 0777);
	if (fd < 0) {
		ERR("Fail, write verify open %s (%d)\n", file, errno);
		free(buf);
		return -EINVAL;
	}

	count = b_length, r_len = 0;

	while (count > 0) {
		ret = read(fd, buf, count);
		if (ret < 0) {
			ERR("Fail, verified %lld (%d)\n", r_len, errno);
			break;
		}

		if (verify) {
			for (i = (r_len ? 0 : 4); b_length/4 > i; i++) {
				if (buf[i] != i) {
					ERR("Fail, verified 0x%llx, not equal 0x%08x vs 0x%08x\n",
					    (r_len + (i*4)),
					    (unsigned int)buf[i], i);
					goto err_write;
				}
			}
		}

		r_len += ret;
		count = f_length - r_len;

		if (b_length < count)
			count = b_length;
	}

err_write:
	close(fd);
	free(buf);

	if (r_len != f_length)
		return -EINVAL;

	return r_len;
}

static long long file_read(const char *file, unsigned long f_flags,
			   long long f_length, int b_length, u64 *time,
			   int verify)
{
	int fd, flags = O_RDONLY;
	unsigned int *buf;
	long long r_len, f_len = 0;
	u64 ts = 0, te;
	int count, b_len = 0, d_len;
	long ret;
	int num;

	if (file_read_sign(file, &f_len, &b_len) < 0)
		return -EINVAL;

	if (f_length > f_len)
		f_length = f_len;

	if (b_length > BUFFER_MAX_SIZE)
		b_length = BUFFER_MAX_SIZE;

	if (b_length > f_length)
		b_length = f_length;

	/* disk cache flush */
	if (!access("/proc/sys/vm/drop_caches", W_OK)) {
		if (f_flags & FILE_O_SYNC)
			ret = system("echo 3 > /proc/sys/vm/drop_caches > /dev/null");
	}

	/*
	 * O_DIRECT로 열린 파일은 기록하려는 메모리 버퍼, 파일의 오프셋,
         * 버퍼의 크기가 모두 디스크 섹터 크기의 배수로 정렬되어 있어야 한다.
	 * 따라서 메모리는posix_memalign을 사용해서 섹터 크기로 정렬되도록
	 * 해야 한다.
	 */
	ret = posix_memalign((void *)&buf, SECTOR_SIZE, b_length);
	if (ret) {
		ERR("Fail: allocate memory buffer %d (%d)\n", b_length, errno);
		return -ENOMEM;
	}

	memset(buf, 0, b_length);

	/* wait for "start of" clock tick */
	if (f_flags & FILE_O_SYNC)
		sync();

	/* verify open */
	flags = FILE_R_FLAG | (f_flags & FILE_O_DIRECT ? O_DIRECT : 0);

	fd = open(file, flags, 0777);
	if (fd < 0) {
		flags = FILE_R_FLAG | (f_flags & FILE_O_SYNC ? O_SYNC : 0);

		fd = open(file, flags, 0777);
		if (fd < 0) {
			flags = FILE_R_FLAG;
			fd = open(file, flags, 0777);
			if (fd < 0) {
				ERR("Fail, read open %s (%d)\n", file, errno);
				free(buf);
				return -EINVAL;
			}
		}
	}

	/* read and verify */
	count = b_length, r_len = 0, num = 0, b_len /= 4, d_len = b_len - 4;

	if (time)
		RUN_TIME_US(ts);

	while (count > 0) {
		ret = read(fd, buf, count);
		if (ret < 0) {
			ERR("Fail, read %lld (%d)\n", r_len, errno);
			break;
		}

		/* verify */
		if (verify) {
			for (num = (r_len ? 0 : 4); count/4 > num; num++) {
				if (!d_len)
					d_len = b_len;

				if (buf[num] != (unsigned int)(b_len - d_len)) {
					ERR("Fail, read 0x%llx, not equal 0x%08x vs 0x%08x ---\n",
					     (r_len + (num * 4)),
					     (unsigned int)buf[num],
					     (unsigned int)(b_len - d_len));
					goto err_read;
				}
				d_len--;
			}
		}

		r_len += ret;
		count = f_length - r_len;

		if (b_length < count)
			count = b_length;
	}

	/* End */
	if (f_flags & FILE_O_SYNC)
		sync();

	if (time) {
		END_TIME_US(ts, te);
		*time = te;
	}

err_read:
	close(fd);
	free(buf);

	if (r_len != f_length)
		return -EINVAL;

	return r_len;
}

static long long parse_length(int argc, char **argv, char *str,
			      long long *min, long long *max,
			      const char *smin, const char *smax,
			      bool *random)
{
	char *s;
	long long MIN = min ? *min : 0;
	long long MAX = max ? *max : 0;
	long long length = 0;
	bool find = false;
	int i;

	if (!str)
		return 0;

	if (strchr(str, 'k') || strchr(str, 'K')) {
		length = KB(strtoll(str, NULL, 10));
	} else if (strchr(str, 'm') || strchr(str, 'M')) {
		length = MB(strtoll(str, NULL, 10));
	} else if ((s = strchr(str, 'r'))) {
		for (i = 0; i < argc; i++) {
			if (s == argv[i])
				find = true;

			if (!find)
				continue;

			s = smin ? strstr(argv[i], smin) : NULL;
			if (s && min) {
				s = s + strlen(smin);
				if (strchr(s, 'k') || strchr(s, 'K'))
					*min = KB(strtoll(s, NULL, 10));
				else if (strchr(s, 'm') || strchr(s, 'M'))
					*min = MB(strtoll(s, NULL, 10));
				else
					*min = strtoll(s, NULL, 10);
			}

			s = smax ? strstr(argv[i], smax) : NULL;
			if (s && max) {
				s = s + strlen(smax);
				if (strchr(s, 'k') || strchr(s, 'K'))
					*max = KB(strtoll(s, NULL, 10));
				else if (strchr(s, 'm') || strchr(s, 'M'))
					*max = MB(strtoll(s, NULL, 10));
				else
					*max = strtoll(s, NULL, 10);
			}
		}

		if (random)
			*random = true;

		if (min && MIN > *min)
			*min = MIN;

		if (min && MAX < *min)
			*min = MAX;

		if (max && MIN > *max)
			*max = MIN;

		if (max && MAX < *max)
			*max = MAX;
	} else {
		/* align with sector size */
		length = (strtoll(str, NULL, 10) + SECTOR_SIZE - 1) /
			  SECTOR_SIZE * SECTOR_SIZE;
	}

	return length;
}

static int test_write(const char *disk, const char *file,
		      ulong f_flags, long long f_length, int b_length,
		      long long *length, int counts, bool verify, u64 *time)
{
	long long disk_avail;
	long long size;

	/*
	 * check disk free
	 */
	disk_avail = disk_disk_avail(disk, NULL, 0);
	if (f_length > disk_avail) {
		int ret = disk_obtain_space(disk, counts, f_length);

		if (ret < 0) {
			MSG("No space left, free %lld, req %lld\n",
			     disk_disk_avail(disk, NULL, 0), f_length);
			return -ENOMEM;
		}
	}

	size = file_write(file, f_flags, f_length, b_length,
			    time, 1, verify);
	if (size < 0) {
		ERR("Fail write file, length %lld\n", size);
		return (int)size;
	}

	if (length)
		*length = size;

	return 0;
}

static int test_read(const char *disk, const char *file,
		     ulong f_flags, long long f_length, int b_length,
		     long long *length, int counts, bool verify, u64 *time)
{
	long long size;

	/*
	 * check exist file
	 */
	if (file_read_sign(file, NULL, NULL) < 0) {
		long long disk_avail = disk_disk_avail(disk, NULL, 0);

		if (disk_avail < f_length) {
			int ret = disk_obtain_space(disk, counts, f_length);

			if (ret < 0) {
				MSG("No space left, free %lld, req %lld\n",
				     disk_disk_avail(disk, NULL, 0), f_length);
				return ret;
			}
		}

		size = file_write(file, f_flags, f_length, b_length,
				    NULL, 0, verify);
		if (size < 0) {
			ERR("Fail write file to read, length %lld\n", size);
			return (int)size;
		}
	}

	/*
	 * read test
	 */
	size = file_read(file, f_flags, f_length, b_length, time, verify);
	if (size < 0) {
		ERR("Fail read length %lld\n", size);
		return (int)size;
	}

	if (length)
		*length = size;

	return 0;
}

static void print_usage(void)
{
	fprintf(stdout, "\n");
	fprintf(stdout, "usage: options\n");
	fprintf(stdout, "-p directory path, default current path\n");
	fprintf(stdout, "-r read  option  , default read\n");
	fprintf(stdout, "-w write option  , default read\n");
	fprintf(stdout, "-b rw buffer len , default %dKbyte (k=Kbyte, m=Mbyte, r=random)\n",
		BUFFER_DEF_SIZE/MBYTE);
	fprintf(stdout, "   bmin=n        , random min, default and limit min  %dbyte\n",
		BUFFER_MIN_SIZE);
	fprintf(stdout, "   bmax=n        , random max, default and limit max %dMbyte\n",
		BUFFER_MAX_SIZE/MBYTE);
	fprintf(stdout, "-f rw file len   , default %dMbyte (k=Kbyte, m=Mbyte, r=random)\n",
		FILE_DEF_SIZE/MBYTE);
	fprintf(stdout, "   fmin=n        , random min, default and limit min %dMbyte\n",
		FILE_MIN_SIZE/MBYTE);
	fprintf(stdout, "   fmax=n        , random max, default and limit max %dMbyte\n",
		FILE_MAX_SIZE/MBYTE);
	fprintf(stdout, "-c test count    , default %d\n", DISK_COUNT);
	fprintf(stdout, "-l loop          ,\n");
	fprintf(stdout, "-s no sync access, default sync\n");
	fprintf(stdout, "-t no time info  ,\n");
	fprintf(stdout, "-n set priority  , FIFO 99\n");
	fprintf(stdout, "-v skip verify\n");
	fprintf(stdout, "\n");
}

/* program options */
struct option_t {
	const char *disk;
	char *buff_size, *file_size;
	int counts;
	unsigned int loop;
	bool rd, wr;
	bool fsync, rt_sched;
	bool verify, timei;
} option = {
	.disk = DISK_PATH,
	.counts = DISK_COUNT,
	.rd = false,
	.wr = false,
	.loop = 0,
	.fsync = true,
	.rt_sched = false,
	.verify = true,
	.timei = true,
};

static void parse_options(int argc, char **argv, struct option_t *op)
{
	int opt;

	while (-1 != (opt = getopt(argc, argv, "hrwp:b:f:c:l:stnv"))) {
		switch (opt) {
		case 'h':
			print_usage(); exit(1);
			break;
		case 'p':
			op->disk = optarg;
			break;
		case 'r':
			op->rd = true;
			break;
		case 'w':
			op->wr = true;
			break;
		case 'b':
			op->buff_size = optarg;
			break;
		case 'f':
			op->file_size = optarg;
			break;
		case 'c':
			op->counts = atoi(optarg);
			break;
		case 'l':
			op->loop = atoi(optarg);
			break;
		case 's':
			op->fsync = false;
			break;
		case 't':
			op->timei = false;
			break;
		case 'n':
			op->rt_sched = true;
			break;
		case 'v':
			op->verify = false;
			break;
		default:
			print_usage(), exit(1);
			break;
		}
	}
}

int main(int argc, char **argv)
{
	char file[256];
	struct option_t *op = &option;
	long long f_len = FILE_DEF_SIZE;
	long long f_min = FILE_MIN_SIZE, f_max = FILE_MAX_SIZE;
	long long b_len = BUFFER_DEF_SIZE;
	long long b_min = BUFFER_MIN_SIZE, b_max = BUFFER_MAX_SIZE;
	ulong f_flags = FILE_O_SYNC | FILE_O_DIRECT;
	bool rand_file_size = false, rand_buff_size = false;
	long long disk_avail;
	struct tm *tm;
	time_t tt;
	int i, count = 0;
	int ret;

	parse_options(argc, argv, op);

	/* get buffer length */
	b_len = parse_length(argc, argv, op->buff_size,
			     &b_min, &b_max, "bmin=", "bmax=", &rand_buff_size);

	if (!rand_buff_size && b_len > BUFFER_MAX_SIZE) {
		ERR("Fail, Invalid buffer %lld, max %d byte\n",
		     b_len, BUFFER_MAX_SIZE);
		print_usage();
		exit(1);
	}

	if (!b_len)
		b_len = BUFFER_DEF_SIZE;

	/* get file length */
	f_len = parse_length(argc, argv, op->file_size,
			     &f_min, &f_max, "fmin=", "fmax=", &rand_file_size);

	if (!f_len)
		f_len = FILE_DEF_SIZE;

	/* default operation */
	if (!op->rd && !op->wr)
		op->rd = true;

	if (!op->fsync)
		f_flags = 0;

	/* random seed */
	srand(time(NULL));

	/* current disk avail */
	disk_avail = disk_disk_avail(op->disk, NULL, 0);

	time(&tt);
	tm = localtime(&tt);

	/* debug message */
	MSG("\n");
	MSG("Disk path   : '%s'\n", op->disk);
	MSG("Disk test   : Read [%s], Write [%s]\n",
	     op->rd ? "Yes" : "No", op->wr ? "Yes" : "No");

	if (rand_file_size)
		MSG("File size   : random, min %lld byte, max %lld byte (free %lld Mbyte)\n",
			f_min, f_max, disk_avail/MBYTE);
	else
		MSG("File size   : %lld byte (free %lld MByte)\n", f_len, disk_avail/MBYTE);

	if (rand_buff_size)
		MSG("Buff size   : random, min %lld byte, max %lld byte\n", b_min, b_max);
	else
		MSG("Buff size   : %lld byte\n", b_len);

	MSG("Sync access : %s\n", op->fsync ? "Yes" : "No");
	MSG("Check time  : %s\n", op->timei ? "Yes" : "No");
	MSG("Test count  : %d\n", op->counts);
	MSG("Test loop   : %d\n", op->loop);
	MSG("Test time   : %d-%d-%d %d:%d:%d\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	MSG("\n");

	/* real time */
	if (op->rt_sched)
		sched_set_new(getpid(), SCHED_FIFO, 99);

	do {
		for (i = 0; i < op->counts; i++) {
			long long length = 0;
			u64 time = 0, *ptime = op->timei ? &time : NULL;

			if (rand_buff_size)
				RAND_SIZE(b_min, b_max, SECTOR_SIZE, b_len);

			if (rand_file_size)
				RAND_SIZE(f_min, f_max, f_min, f_len);

			sprintf(file, "%s/%s.%d.txt", op->disk, FILE_PREFIX, i);
			MSG("Test file   : %s, count [%3d/%3d]\n",
			     basename(file), i, count);

			if (op->wr) {
				ret = test_write(op->disk, file, f_flags,
						f_len, b_len, &length,
						op->counts, op->verify, ptime);
				if (ret < 0)
					return ret;

				MSG("Test write  : %3lld.%06lld, %lld/%lld (%3lld.%6lld M/S)\n",
					time ? SE(time) : 0, time ? US(time) : 0, f_len, b_len,
					time ? MBS(length, time) : 0, time ? MBU(length, time) : 0);
			}

			if (op->rd) {
				ret = test_read(op->disk, file, f_flags,
						f_len, b_len, &length,
						op->counts, op->verify, ptime);
				if (ret < 0)
					return ret;
				MSG("Test read   : %3lld.%06lld, %lld/%lld (%3lld.%6lld M/S)\n",
					time ? SE(time) : 0, time ? US(time) : 0, f_len, b_len,
					time ? MBS(length, time) : 0, time ? MBU(length, time) : 0);
			}
			MSG("\n");
		}
		count++;
	} while (count < op->loop);

	return 0;
}
