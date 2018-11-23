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
#include <sys/signal.h>

#define	TEST_SIGN		0xD150D150
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
#define SECTOR_SIZE 		KB(1)

#define	FILE_O_SYNC		(1<<0)
#define	FILE_O_DIRECT		(1<<1)

#define	FILE_W_FLAG		(O_RDWR | O_CREAT)
#define	FILE_R_FLAG		(O_RDONLY)

typedef unsigned long long  u64;

#define	_SE_(_us)		(_us/1000000)
#define	_US_(_us)		(_us%1000000)

#define	_MB_PER_S_(_l, _u) 	((((u64)_l/(u64)_u)*1000000)/(u64)MBYTE)
#define	_MB_PER_U_(_l, _u) 	((((u64)_l/(u64)_u)*1000000)%(u64)MBYTE)

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


static int disk_new_sched(pid_t pid, int policy, int priority)
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
			printf("invalid policy %d (0~3)\n", policy);
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
		printf("\nFail, invalid priority (%d ~ %d)...\n",
			minpri, maxpri);
		return -EINVAL;
	}

	if (policy == SCHED_NORMAL) {
		param.sched_priority = 0;
		ret = sched_setscheduler(pid, policy, &param);
		if (ret) {
			printf("Fail, sched_setscheduler(ret %d), %s\n\n",
				ret, strerror(errno));
			return ret;
		}
		ret = setpriority(PRIO_PROCESS, pid, priority);
		if (ret)
			printf("Fail, setpriority(ret %d), %s\n\n",
				ret, strerror(errno));
	} else {
		param.sched_priority = priority;
		ret = sched_setscheduler(pid, policy, &param);
		if (ret)
			printf("Fail, sched_setscheduler(ret %d), %s\n\n",
				ret, strerror(errno));
	}

	return ret;
}

static long long disk_disk_avail(char *disk, long long *ptotal, int debug)
{
    	long long total, used, free, avail;
	double used_percent;
	struct statfs64 fs;
	struct stat st;

	if (stat(disk, &st)) {
		if (mkdir(disk, 0755)) {
	        	printf ("Fail, make dir path = %s \n", disk);
        		return 0;
	    	}
	} else {
		if (!S_ISDIR(st.st_mode)) {
       		 	printf("Fail, file exist %s, %s ...\n",
				disk, strerror(errno));
			return 0;
    		}
	}

	if (statfs64(disk, &fs) < 0) {
		printf("Fail, status fs %s, %s\n", disk, strerror(errno));
		return 0;
	}

	total = (fs.f_bsize * fs.f_blocks);
	free = (fs.f_bsize * fs.f_bavail);
	avail = (fs.f_bsize * fs.f_bavail);
	used = (total - free);
	used_percent = ((double)used/(double)total)*100;

	if (debug) {
		printf("--------------------------------------\n");
		printf("PATH       : %s\n", disk);
    		printf("Total size : %16lld  \n", total);
	    	printf("Free  size : %16lld  \n", free);
   		printf("Avail size : %16lld  \n", avail);
		printf("Used  size : %16lld  \n", used);
	    	printf("Block size : %8d B \n", (int)fs.f_bsize);
    		printf("Use percent: %16f %% \n", used_percent);
	    	printf("--------------------------------------\n");
	}

	if (ptotal)
		*ptotal = total;

    return avail;
}

static int disk_obtain_space(char *disk, int search, long long request)
{
	char file[256];
	long long avail;
	int  index = 0;

	while (1) {
		/* exist file */
		sprintf(file, "%s/%s.%d.txt", disk, FILE_PREFIX, index);

		if (access(file, F_OK) < 0){
			if (index > search) {
				printf("Fail, access %s, %s\n", file, strerror(errno));
				return -EINVAL;
			}
			index++; /* next file */
			continue;
		}

		/* remove to obtain free */
		if (remove(file) < 0) {
			printf("Fail, remove %s, %s\n", file, strerror(errno));
			return -EINVAL;
		}
		sync();

		/* check free */
		avail = disk_disk_avail(disk, NULL, 0);
		printf("remove file: %s, avail %lld, req %lld ...\n", file, avail, request);
		if (avail > request)
			break;
		index++; /* next file */
	}

	return 0;
}

static int disk_get_info(char *disk, long long *pf_length, int *pb_length)
{
	unsigned int data[4] = { 0, };
	long long f_size;
	int fd, b_size;
	int ret;

	fd = open(disk, O_RDWR|O_SYNC, 0777);
	if (fd < 0) {
		fd = open(disk, O_RDWR, 0777);
		if (fd < 0) {
			printf("Fail, info open %s, %s \n",
				disk, strerror(errno));
			return -EINVAL;
		}
	}

   	ret = read(fd, (void*)data, sizeof(data));
	close(fd);

	if (data[0] != TEST_SIGN){
		printf("Fail, Unknown signature 0x%08x (0x%08x)\n",
			data[0], TEST_SIGN);
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

static int disk_set_info(char *disk, long long f_length, int b_length)
{
	unsigned int data[4];
	int fd;

	fd = open(disk, O_RDWR|O_SYNC, 0777);
	if (fd < 0) {
       		printf("Fail, info open %s, %s \n", disk, strerror(errno));
		return -EINVAL;
   	}

	data[0] = TEST_SIGN;
	data[1] = b_length;
	data[2] = (f_length) & 0xFFFFFFFF;
	data[3] = (f_length >> 32) & 0xFFFFFFFF;

    	write(fd, (void*)&data, sizeof(data));
	close(fd);

	sync();

	return 0;
}

static long long disk_write(char *disk, unsigned long f_flags,
			    long long f_length, int b_length,
			    long long *pf_length, int *pb_length, u64 *time, int wo,
			    int verify)
{
	int fd, flags = O_RDWR | O_CREAT;
	int *buf;
	long long w_len, r_len;
	u64 ts, te;
	int count;
	int i, ret;

	if (b_length > BUFFER_MAX_SIZE)
		b_length = BUFFER_MAX_SIZE;

	if (b_length > f_length)
		b_length = f_length;

	if (pf_length)
		*pf_length = f_length;

	if (pb_length)
		*pb_length = b_length;

	/*
	 * O_DIRECT로 열린 파일은 기록하려는 메모리 버퍼, 파일의 오프셋, 버퍼의 크기가
	 * 모두 디스크 섹터 크기의 배수로 정렬되어 있어야 한다. 따라서 메모리는
	 * posix_memalign을 사용해서 섹터 크기로 정렬되도록 해야 한다.
	 */
	ret = posix_memalign((void*)&buf, SECTOR_SIZE, b_length);
   	if (ret) {
		printf("Fail: allocate memory for buffer %d: %s\n",
			b_length, strerror(ret));
		return -ENOMEM;
	}

	/* fill buffer */
	for (i = 0; i < b_length/4; i++)
		buf[i] = i;

	/* wait for "start of" clock tick */
	if (f_flags & FILE_O_SYNC)
		sync();

	flags = FILE_W_FLAG | (f_flags & FILE_O_DIRECT ? O_DIRECT : 0);

	fd = open(disk, flags, 0777);
	if (fd < 0) {
		flags = FILE_W_FLAG | (f_flags & FILE_O_SYNC ? O_SYNC : 0);

		fd = open(disk, flags, 0777);
		if (fd < 0) {
			flags = FILE_W_FLAG;

			fd = open(disk, flags, 0777);
			if (0 > fd) {
				printf("Fail, write open %s, %s\n",
					disk, strerror(errno));
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
			printf("Fail, wrote %lld, %s\n", w_len, strerror(errno));
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

	if (time){
		END_TIME_US(ts, te);
		*time = te;
	}

	close(fd);

	if (w_len != f_length) {
		free(buf);
		return -EINVAL;
	}

	/* set test file info */
	if (disk_set_info(disk, f_length, b_length) < 0)
		return -EINVAL;

	if (wo)
		return f_length;

	/* verify open */
	flags = O_RDONLY | (flags & ~(FILE_W_FLAG));

	fd = open(disk, flags, 0777);
	if (fd < 0) {
	       	printf("Fail, write verify open %s, %s\n",
			disk, strerror(errno));
		free(buf);
		return -EINVAL;
   	}

	count = b_length, r_len = 0;

	while (count > 0) {
		ret = read(fd, buf, count);
		if (ret < 0) {
			printf("Fail, verified %lld, %s \n",
				r_len, strerror(errno));
			break;
		}

		if (verify) {
			for (i = (r_len ? 0: 4); b_length/4 > i; i++) {
				if (buf[i] != i) {
					printf("Fail, verified 0x%llx, not equal data=0x%08x with req=0x%08x\n",
						(r_len+(i*4)), (unsigned int)buf[i], i);
					goto err_write;
				}
			}
		}

		r_len += ret;
		count = f_length - r_len;
		if (count > b_length)
			count = b_length;
	}

err_write:
	close(fd);
	free(buf);

	if (r_len != f_length)
		return -EINVAL;

	return r_len;
}

static long long disk_read(char *disk, unsigned long f_flags,
			   long long f_length, int b_length,
			   long long *pf_length, int *pb_length, u64 *time,
			   int verify)
{
	int fd, flags = O_RDONLY;
	long ret;
	unsigned int *buf;
	long long r_len, f_len = 0;
	u64 ts, te;
	int count, b_len = 0, d_len;
	int num;

	if (disk_get_info(disk, &f_len, &b_len) < 0)
		return -EINVAL;

	if (f_length > f_len)
		f_length = f_len;

	if (b_length > BUFFER_MAX_SIZE)
		b_length = BUFFER_MAX_SIZE;

	if (b_length > f_length)
		b_length = f_length;

	if (pf_length)
		 *pf_length = f_length;

	if (pb_length)
		 *pb_length = b_length;

#if 1
	/* disk cache flush */
	if (f_flags & FILE_O_SYNC)
		ret = system("echo 3 > /proc/sys/vm/drop_caches > /dev/null");
#endif

	/*
	 * O_DIRECT로 열린 파일은 기록하려는 메모리 버퍼, 파일의 오프셋, 버퍼의 크기가
	 * 모두 디스크 섹터 크기의 배수로 정렬되어 있어야 한다. 따라서 메모리는
	 * posix_memalign을 사용해서 섹터 크기로 정렬되도록 해야 한다.
	 */
	ret = posix_memalign((void*)&buf, SECTOR_SIZE, b_length);
   	if (ret) {
	       printf("Fail: allocate memory for buffer %d: %s\n",
			b_length, strerror(ret));
		return -ENOMEM;
   	}

	memset(buf, 0, b_length);

	/* wait for "start of" clock tick */
	if (f_flags & FILE_O_SYNC)
		sync();

	/* verify open */
	flags = FILE_R_FLAG | (f_flags & FILE_O_DIRECT ? O_DIRECT : 0);

	fd = open(disk, flags, 0777);
	if (fd < 0) {
		flags = FILE_R_FLAG | (f_flags & FILE_O_SYNC ? O_SYNC : 0);

		fd = open(disk, flags, 0777);
		if (fd < 0) {
			flags = FILE_R_FLAG;
			fd = open(disk, flags, 0777);
			if (fd < 0) {
		       		printf("Fail, read open %s, %s\n",
					disk, strerror(errno));
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
			printf("Fail, read %lld, %s \n", r_len, strerror(errno));
			break;
		}

		/* verify */
		if (verify) {
			for (num = (r_len ? 0: 4); count/4 > num; num++) {
				if (!d_len)
					d_len = b_len;

				if (buf[num] != (unsigned int)(b_len - d_len)) {
					printf("Fail, read 0x%llx, not equal data=0x%08x with req=0x%08x ---\n",
						(r_len + (num * 4)), (unsigned int)buf[num], (unsigned int)(b_len - d_len));
					goto err_read;
				}
				d_len--;
			}
		}

		r_len += ret;
		count = f_length - r_len;

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

err_read:
	close(fd);
	free(buf);

	if (r_len != f_length)
		return -EINVAL;

	return r_len;
}

static long long parse_strtol(int argc, char **argv, const char *string,
			      long long min, long long max, bool *random)
{
	char *s;
	long long length = 0;
	bool find = false;
	int i;

	if (strchr(string, 'k') || strchr(string, 'K')) {
		length = KB(strtoll(string, NULL, 10));
	} else if (strchr(string, 'm') || strchr(string, 'M')) {
		length = MB(strtoll(string, NULL, 10));
	} else if (s = strchr(string, 'r')) {
		*random = true;
		for (i = 0; i < argc; i++) {
			if (s == argv[i])
				find = true;

			if (!find)
				continue;

			if (s = strstr(argv[i], "bmin=")) {
				s = s + strlen("bmin=");
				if (strchr(s, 'k') || strchr(s, 'K'))
					min = KB(strtoll(s, NULL, 10));
				else if (strchr(s, 'm') || strchr(s, 'M'))
					min = MB(strtoll(s, NULL, 10));
				else
					min = strtoll(s, NULL, 10);
			}
			if (s = strstr(argv[i], "bmax=")) {
				s = s + strlen("bmax=");
				if (strchr(s, 'k') || strchr(s, 'K'))
					max = KB(strtoll(s, NULL, 10));
				else if (strchr(s, 'm') || strchr(s, 'M'))
					max = MB(strtoll(s, NULL, 10));
				else
					max = strtoll(s, NULL, 10);
			}
		}
	} else {
		length = strtoll(string, NULL, 10);
		length = (length + SECTOR_SIZE - 1)/SECTOR_SIZE * SECTOR_SIZE;
	}

	return length;
}

static void print_usage(void)
{
	printf("\n");
	printf("usage: options\n");
	printf("-p directory path, default current path\n");
	printf("-r read  option  , default read\n");
	printf("-w write option  , default read\n");
	printf("-b rw buffer len , default %dKbyte (k=Kbyte, m=Mbyte, r=random) \n",
		BUFFER_DEF_SIZE/MBYTE);
	printf("   bmin=n        , random min, default and limit min  %dbyte\n",
		BUFFER_MIN_SIZE);
	printf("   bmax=n        , random max, default and limit max %dMbyte\n",
		BUFFER_MAX_SIZE/MBYTE);
	printf("-l rw file len   , default %dMbyte (k=Kbyte, m=Mbyte, r=random) \n",
		FILE_DEF_SIZE/MBYTE);
	printf("   lmin=n        , random min, default and limit min %dMbyte\n",
		FILE_MIN_SIZE/MBYTE);
	printf("   lmax=n        , random max, default and limit max %dMbyte\n",
		FILE_MAX_SIZE/MBYTE);
	printf("-c test count    , default %d\n", DISK_COUNT);
	printf("-f loop          ,\n");
	printf("-s no sync access, default sync\n");
	printf("-t no time info  ,\n");
	printf("-n set priority  , FIFO 99\n");
	printf("-o file name to save test info\n");
	printf("-v skip verify\n");
	printf("\n");
}

#define RAND_SIZE(min, max, aln, val) { \
		val = rand() % max; \
		val = ((val+aln-1)/aln)*aln; \
		if (min > val) val = min; \
	}

static FILE *debug_out;
#define	MSG(exp...)	{ fprintf(debug_out, exp); fflush(debug_out); }

/* program options */
struct option_t {
	char *disk;
	char *buff_size, *file_size;
	char *dbg_out;
	int counts;
	bool rd, wr;
	bool loop, fsync, rt_sched;
	bool verify, timei;
} option = {
	.disk = DISK_PATH,
	.counts = DISK_COUNT,
	.rd = false,
	.wr = false,
	.loop = false,
	.fsync = true,
	.rt_sched = false,
	.verify = true,
	.timei = true,
};

static void parse_options(int argc, char **argv, struct option_t *op)
{
	int opt;

	while (-1 != (opt = getopt(argc, argv, "hrwp:b:l:c:sftno:v"))) {
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
			case 'l':
				op->file_size = optarg;
				break;
			case 'c':
				op->counts = atoi(optarg);
				break;
			case 'f':
				op->loop = true;
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
			case 'o':
				op->dbg_out = optarg;
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
	struct option_t *op = &option;

	char *pav = NULL;
	char file[256];
	long long f_len = FILE_DEF_SIZE;
	long long f_min = FILE_MIN_SIZE, f_max = FILE_MAX_SIZE;
	int b_len = BUFFER_DEF_SIZE;
	int b_min = BUFFER_MIN_SIZE, b_max = BUFFER_MAX_SIZE;
	ulong f_flags = FILE_O_SYNC | FILE_O_DIRECT;
	bool rand_f_len = false, rand_b_len = false;
	long long disk_avail;
	int index = 0, count = 0;
	int i, ret;

	parse_options(argc, argv, op);

	/* check buffer options */
	if (op->buff_size) {
		if (strchr(op->buff_size, 'k') || strchr(op->buff_size, 'K')) {
			b_len = KB(atoi(op->buff_size));
		} else if (strchr(op->buff_size, 'm') || strchr(op->buff_size, 'M')) {
			b_len = MB(atoi(op->buff_size));
		} else if (pav = strchr(op->buff_size, 'r')) {
			bool find = false;
			rand_b_len  = true;
			for (i = 0; i < argc; i++) {
				if (pav == argv[i])	find = true;
				if (false == find)		continue;
				if (pav = strstr(argv[i], "bmin=")) {
					pav = pav+strlen("bmin=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						b_min = KB(atoi(pav));
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						b_min = MB(atoi(pav));
					else
						b_min = atoi(pav);
				}
				if (pav = strstr(argv[i], "bmax=")) {
					pav = pav+strlen("bmax=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						b_max = KB(atoi(pav));
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						b_max = MB(atoi(pav));
					else
						b_max = atoi(pav);
				}
			}
		} else {
			b_len = atoi(op->buff_size);
			b_len = (b_len + SECTOR_SIZE -1)/SECTOR_SIZE * SECTOR_SIZE;
		}
	}

	/* file length options */
	if (op->file_size) {
		if (strchr(op->file_size, 'k') || strchr(op->file_size, 'K')) {
			f_len = (long long)KB(atoi(op->file_size));
		} else if (strchr(op->file_size, 'm') || strchr(op->file_size, 'M')) {
			f_len = (long long)MB(atoi(op->file_size));
		} else if (pav = strchr(op->file_size, 'r')) {
			bool find = false;

			rand_f_len = true;
			for (i = 0; argc > i; i++) {
				if (pav == argv[i])	find = true;
				if (false == find)		continue;
				if (pav = strstr(argv[i], "lmin=")) {
					pav = pav+strlen("lmin=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						f_min = KB(atoi(pav));
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						f_min = MB(atoi(pav));
					else
						f_min = atoi(pav);
				}
				if (pav = strstr(argv[i], "lmax=")) {
					pav = pav+strlen("lmax=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						f_max = KB(atoi(pav));
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						f_max = MB(atoi(pav));
					else
						f_max = atoi(pav);
				}
			}
		} else {
			f_len = atoi(op->file_size);
			f_len = (f_len + SECTOR_SIZE -1)/SECTOR_SIZE * SECTOR_SIZE;
		}
	}

	/* output file */
	if (op->dbg_out) {
		debug_out = fopen(op->dbg_out, "w+");
		if (!debug_out) {
       			printf("Fail, output open %s, %s\n", op->dbg_out, strerror(errno));
			exit(0);
   		}
   		printf("----- save test result to '%s' -----\n", op->dbg_out);
   		sync();
	} else {
		debug_out = stdout;
	}

	/* r/w buffer */
	if (rand_b_len) {
		if (BUFFER_MIN_SIZE > b_min) b_min = BUFFER_MIN_SIZE;
		if (BUFFER_MAX_SIZE < b_min) b_min = BUFFER_MAX_SIZE;
		if (BUFFER_MIN_SIZE > b_max) b_max = BUFFER_MIN_SIZE;
		if (BUFFER_MAX_SIZE < b_max) b_max = BUFFER_MAX_SIZE;
	} else {
		if (b_len > BUFFER_MAX_SIZE) {
			printf("Fail, Invalid buffer len, max %d byte\n", b_len, BUFFER_MAX_SIZE);
			print_usage();
			exit(0);
		}
	}

	/* file length */
	if (rand_f_len) {
		if (FILE_MIN_SIZE > f_min) f_min = FILE_MIN_SIZE;
		if (FILE_MAX_SIZE < f_min) f_min = FILE_MAX_SIZE;
		if (FILE_MIN_SIZE > f_max) f_max = FILE_MIN_SIZE;
		if (FILE_MAX_SIZE < f_max) f_max = FILE_MAX_SIZE;
	}

	/* real time */
	if (op->rt_sched)
		disk_new_sched(getpid(), SCHED_FIFO, 99);

	/* default operation */
	if (!op->rd && !op->wr)
		op->rd = true;

	if (!op->fsync)
		f_flags = 0;

	/* random seed */
	srand(time(NULL));

	/* current disk avail */
	disk_avail = disk_disk_avail(op->disk, NULL, 0);

	/* debug message */
	MSG("\n");
	MSG("disk path  : '%s'\n", op->disk);
	MSG("disk test  : R[%s], W[%s]\n",
	     op->rd ? "Y" : "N", op->wr ? "Y" : "N");

	if (rand_f_len) {
		MSG("File size  : random, min %lld byte, max %lld byte (free disk %lld byte)\n",
			f_min, f_max, disk_avail);
	} else {
		MSG("File size  : %8lld byte (free %lld byte)\n", f_len, disk_avail);
	}

	if (rand_b_len) {
		MSG("Buff size  : random, min %d byte, max %d byte\n", b_min, b_max);
	} else {
		MSG("Buff size  : %8d byte       \n", b_len);
	}

	MSG("sync access: %s\n", op->fsync ? "Y" : "N");
	MSG("check time : %s\n", op->timei ? "Y" : "N");
	MSG("Test count : %d\n", op->counts);
	MSG("Test loop  : %s\n", op->loop ? "Y":" N");
	MSG("\n");

	do {
		for (index = 0; index < op->counts; index++) {

			long long length = 0, readl = 0, writel = 0;
			u64 time = 0, *ptime = op->timei ? &time : NULL;
			int buffer = 0;
			struct tms ts;
			struct tm *tm;
			clock_t clk;

			if (rand_b_len)
				RAND_SIZE(b_min , b_max, SECTOR_SIZE, b_len);
			if (rand_f_len)
				RAND_SIZE(f_min, f_max, f_min, f_len);

			clk = times(&ts);
			tm  = localtime(&clk);
			MSG("Test time  : %d-%d-%d %d:%d:%d\n",
				tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

			sprintf(file, "%s/%s.%d.txt", op->disk, FILE_PREFIX, index);
			MSG("Test path  : %s, count[%4d.%4d] \n", file, count, index);

			if (op->wr) {
				/* check disk free */
				disk_avail = disk_disk_avail(op->disk, NULL, 0);
				if (f_len > disk_avail) {
					ret = disk_obtain_space(op->disk, op->counts, f_len);
					if (ret < 0) {
						MSG("No space left on device, free %lld, req %lld\n",
							disk_disk_avail(op->disk, NULL, 0), f_len);
						exit(1);
					}
				}

				/*
				 * write test
				 */
				writel = disk_write(file, f_flags, f_len, b_len, &length, &buffer, ptime, 1, op->verify);
				if (writel < 0) {
					printf("Fail write file, length %lld (%lld)\n", writel, length);
					exit(1);
				}
				MSG("Test write : %3lld.%06lld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
					time?_SE_(time):0, time?_US_(time):0, length, buffer,
					time?_MB_PER_S_(writel, time):0, time?_MB_PER_U_(writel, time):0);
			}

			if (op->rd) {
				/* check exist file */
				if (disk_get_info(file, NULL, NULL) < 0) {
					disk_avail = disk_disk_avail(op->disk, NULL, 0);
					if (f_len > disk_avail) {
						ret = disk_obtain_space(op->disk, op->counts, f_len);
						if (ret < 0) {
							MSG("No space left on device, free %lld, req %lld \n",
								disk_disk_avail(op->disk, NULL, 0), f_len);
							exit(1);
						}
					}

					writel = disk_write(file, f_flags, f_len, b_len, NULL, NULL, NULL, 0, op->verify);
					if (writel < 0){
						printf("Fail write file to read, length %lld (%lld)\n", writel, length);
						exit(1);
					}
				}

				/*
				 * read test
				 */
				readl = disk_read(file, f_flags, f_len, b_len, &length, &buffer, ptime, op->verify);
				if (readl < 0) {
					printf("Fail read length %lld (%lld)\n", readl, length);
					exit(1);
				}
				MSG("Test read  : %3lld.%06lld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
					time?_SE_(time):0, time?_US_(time):0, length, buffer,
					time?_MB_PER_S_(readl, time):0, time?_MB_PER_U_(readl, time):0);
			}
			MSG("\n");
		}
		count++;
	} while(op->loop);

	if (debug_out)
		fclose(debug_out);

	return 0;
}
