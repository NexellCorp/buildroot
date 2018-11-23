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
#define	_KB_			(1024)
#define	_MB_			(1024 * _KB_)
#define	_GBYTE_			(1024 * _MB_)

#define	BUFFER_DEF_SIZE		(_MB_)
#define	BUFFER_MIN_SIZE		(4)
#define	BUFFER_MAX_SIZE		(50*_MB_)
#define	FILE_DEF_SIZE		BUFFER_MAX_SIZE
#define	FILE_MIN_SIZE		(_MB_)
#define	FILE_MAX_SIZE		(100*_MB_)
#define	SPARE_SIZE		(5*_MB_)

#define	FILE_PREFIX		"test"
#define	DISK_PATH		"./"
#define	DISK_COUNT		(1)
#define SECTOR_SIZE 		(_KB_)

#define	FILE_O_SYNC		(1<<0)
#define	FILE_O_DIRECT		(1<<1)

#define	FILE_W_FLAG		(O_RDWR | O_CREAT)
#define	FILE_R_FLAG		(O_RDONLY)

typedef unsigned long long  u64;

#define	_SE_(_us)		(_us/1000000)
#define	_US_(_us)		(_us%1000000)

#define	_MB_PER_S_(_l, _u) 	((((u64)_l/(u64)_u)*1000000)/(u64)_MB_)
#define	_MB_PER_U_(_l, _u) 	((((u64)_l/(u64)_u)*1000000)%(u64)_MB_)

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

static long long disk_disk_avail(char *path, long long *ptotal, int debug)
{
    	long long total, used, free, avail;
	double used_percent;
	struct statfs64 fs;
	struct stat st;

	if (stat(path, &st)) {
		if (mkdir(path, 0755)) {
	        	printf ("Fail, make dir path = %s \n", path);
        		return 0;
	    	}
	} else {
		if (!S_ISDIR(st.st_mode)) {
       		 	printf("Fail, file exist %s, %s ...\n",
				path, strerror(errno));
			return 0;
    		}
	}

	if (statfs64(path, &fs) < 0) {
		printf("Fail, status fs %s, %s\n", path, strerror(errno));
		return 0;
	}

	total = (fs.f_bsize * fs.f_blocks);
	free = (fs.f_bsize * fs.f_bavail);
	avail = (fs.f_bsize * fs.f_bavail);
	used = (total - free);
	used_percent = ((double)used/(double)total)*100;

	if (debug) {
		printf("--------------------------------------\n");
		printf("PATH       : %s\n", path);
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

static int disk_obtain_space(char *path, int search, long long request)
{
	char file[256];
	long long avail;
	int  index = 0;

	while (1) {
		/* exist file */
		sprintf(file, "%s/%s.%d.txt", path, FILE_PREFIX, index);

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
		avail = disk_disk_avail(path, NULL, 0);
		printf("remove file: %s, avail %lld, req %lld ...\n", file, avail, request);
		if (avail > request)
			break;
		index++; /* next file */
	}

	return 0;
}

static int disk_get_info(char *path, long long *pf_length, int *pb_length)
{
	unsigned int data[4] = { 0, };
	long long f_size;
	int fd, b_size;
	int ret;

	fd = open(path, O_RDWR|O_SYNC, 0777);
	if (fd < 0) {
		fd = open(path, O_RDWR, 0777);
		if (fd < 0) {
			printf("Fail, info open %s, %s \n",
				path, strerror(errno));
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

static int disk_set_info(char *path, long long f_length, int b_length)
{
	unsigned int data[4];
	int fd;

	fd = open(path, O_RDWR|O_SYNC, 0777);
	if (fd < 0) {
       		printf("Fail, info open %s, %s \n", path, strerror(errno));
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

static long long disk_write(char *path, unsigned long fflags,
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
	if (fflags & FILE_O_SYNC)
		sync();

	flags = FILE_W_FLAG | (fflags & FILE_O_DIRECT ? O_DIRECT : 0);

	fd = open(path, flags, 0777);
	if (fd < 0) {
		flags = FILE_W_FLAG | (fflags & FILE_O_SYNC ? O_SYNC : 0);

		fd = open(path, flags, 0777);
		if (fd < 0) {
			flags = FILE_W_FLAG;

			fd = open(path, flags, 0777);
			if (0 > fd) {
				printf("Fail, write open %s, %s\n",
					path, strerror(errno));
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
	if (fflags & FILE_O_SYNC)
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
	if (disk_set_info(path, f_length, b_length) < 0)
		return -EINVAL;

	if (wo)
		return f_length;

	/* verify open */
	flags = O_RDONLY | (flags & ~(FILE_W_FLAG));

	fd = open(path, flags, 0777);
	if (fd < 0) {
	       	printf("Fail, write verify open %s, %s\n",
			path, strerror(errno));
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

static long long disk_read(char *path, unsigned long fflags,
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

	if (disk_get_info(path, &f_len, &b_len) < 0)
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
	if (fflags & FILE_O_SYNC)
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
	if (fflags & FILE_O_SYNC)
		sync();

	/* verify open */
	flags = FILE_R_FLAG | (fflags & FILE_O_DIRECT ? O_DIRECT : 0);

	fd = open(path, flags, 0777);
	if (fd < 0) {
		flags = FILE_R_FLAG | (fflags & FILE_O_SYNC ? O_SYNC : 0);

		fd = open(path, flags, 0777);
		if (fd < 0) {
			flags = FILE_R_FLAG;
			fd = open(path, flags, 0777);
			if (fd < 0) {
		       		printf("Fail, read open %s, %s\n",
					path, strerror(errno));
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
						(r_len+(num*4)), (unsigned int)buf[num], (unsigned int)(b_len-d_len));
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
	if (fflags & FILE_O_SYNC)
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


void print_usage(void)
{
	printf("\n");
	printf("usage: options\n");
	printf("-p directory path, default current path\n");
	printf("-r read  option  , default read\n");
	printf("-w write option  , default read\n");
	printf("-b rw buffer len , default %dKbyte (k=Kbyte, m=Mbyte, r=random) \n",
		BUFFER_DEF_SIZE/_MB_);
	printf("   bmin=n        , random min, default and limit min  %dbyte\n",
		BUFFER_MIN_SIZE);
	printf("   bmax=n        , random max, default and limit max %dMbyte\n",
		BUFFER_MAX_SIZE/_MB_);
	printf("-l rw file len   , default %dMbyte (k=Kbyte, m=Mbyte, r=random) \n",
		FILE_DEF_SIZE/_MB_);
	printf("   lmin=n        , random min, default and limit min %dMbyte\n",
		FILE_MIN_SIZE/_MB_);
	printf("   lmax=n        , random max, default and limit max %dMbyte\n",
		FILE_MAX_SIZE/_MB_);
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
		val = rand()%max; \
		val = ((val+aln-1)/aln)*aln; \
		if (min > val) val = min; \
	}

static FILE *fout = NULL;
#define	DBG(exp...)		{ fprintf(fout, exp); fflush(fout); }

/* program options */
typedef struct option_t {
	char *disk;
	char *buff_size, *file_size;
	char *dbgout;
	int counts;
	bool rd, wr;
	bool loop, fsync, rt_sched;
	bool verify, timei;
} OP_T;

static OP_T *parse_options(int argc, char **argv, OP_T *op)
{
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
				op->buff_size = optarg, o_buf = 1;
				break;
			case 'l':
				op->file_size = optarg, o_len = 1;
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
				op->dbgout = optarg;
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
	int   opt;
	char *path  = DISK_PATH;
	char *pav = NULL, *pab = NULL, *pal = NULL, *d_out = NULL;

	int run_count = DISK_COUNT;
	int b_length = BUFFER_DEF_SIZE, buf_min = BUFFER_MIN_SIZE, buf_max = BUFFER_MAX_SIZE;
	int lrand = 0, brand = 0;
	long long f_length = FILE_DEF_SIZE, file_min = FILE_MIN_SIZE, file_max = FILE_MAX_SIZE;
	long long disk_free;

	int o_buf = 0, o_len = 0;
	int o_sync = 1, o_direct = 1;
	int o_rt = 0, o_loop = 0, o_time = 1;
	int o_wr  = 0, o_rd  = 0, o_out = 0, o_verify = 1;
	int i = 0, n = 0;
	ulong fflags = FILE_O_SYNC | FILE_O_DIRECT;

	char  file[256];
	int   index = 0, loop_count = 0;
	int   ret = 0;

	while (-1 != (opt = getopt(argc, argv, "hrwp:b:l:c:sftno:v"))) {
		switch (opt) {
			case 'h':
				print_usage(); exit(0);
				break;
			case 'p':
				path = optarg;
				break;
			case 'r':
				o_rd = 1;
				break;
			case 'w':
				o_wr = 1;
				break;
			case 'b':
				pab = optarg, o_buf = 1;
				break;
			case 'l':
				pal = optarg, o_len = 1;
				break;
			case 'c':
				run_count = atoi(optarg);
				break;
			case 'f':
				o_loop = 1;
				break;
			case 's':
				o_sync = 0;
				break;
			case 't':
				o_time = 0;
				break;
			case 'n':
				o_rt = 0;
				break;
			case 'o':
				d_out = optarg;
				break;
			case 'v':
				o_verify = 0;
				break;
			default:
				print_usage(), exit(0);
				break;
		}
	}

	/* check buffer options */
	if (o_buf) {
		if (strchr(pab, 'k') || strchr(pab, 'K')) {
			b_length = atoi(pab) * _KB_;
		} else if (strchr(pab, 'm') || strchr(pab, 'M')) {
			b_length = atoi(pab) * _MB_;
		} else if (pav = strchr(pab, 'r')) {
			bool find = false;
			brand  = 1;
			for (i = 0; argc > i; i++) {
				if (pav == argv[i])	find = true;
				if (false == find)		continue;
				if (pav = strstr(argv[i], "bmin=")) {
					pav = pav+strlen("bmin=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						buf_min = atoi(pav) * _KB_;
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						buf_min = atoi(pav) * _MB_;
					else
						buf_min = atoi(pav);
				}
				if (pav = strstr(argv[i], "bmax=")) {
					pav = pav+strlen("bmax=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						buf_max = atoi(pav) * _KB_;
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						buf_max = atoi(pav) * _MB_;
					else
						buf_max = atoi(pav);
				}
			}
		} else {
			b_length = atoi(pab);
			b_length = (b_length + SECTOR_SIZE -1)/SECTOR_SIZE * SECTOR_SIZE;
		}
	}

	/* file length options */
	if (o_len) {
		if (strchr(pal, 'k') || strchr(pal, 'K')) {
			f_length = (long long)atoi(pal) * _KB_;
		} else if (strchr(pal, 'm') || strchr(pal, 'M')) {
			f_length = (long long)atoi(pal) * _MB_;
		} else if (pav = strchr(pal, 'r')) {
			bool find = false;
			lrand  = 1;
			for (i = 0; argc > i; i++) {
				if (pav == argv[i])	find = true;
				if (false == find)		continue;
				if (pav = strstr(argv[i], "lmin=")) {
					pav = pav+strlen("lmin=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						file_min = atoi(pav) * _KB_;
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						file_min = atoi(pav) * _MB_;
					else
						file_min = atoi(pav);
				}
				if (pav = strstr(argv[i], "lmax=")) {
					pav = pav+strlen("lmax=");
					if (strchr(pav, 'k') || strchr(pav, 'K'))
						file_max = atoi(pav) * _KB_;
					else if (strchr(pav, 'm') || strchr(pav, 'M'))
						file_max = atoi(pav) * _MB_;
					else
						file_max = atoi(pav);
				}
			}
		} else {
			f_length = atoi(pal);
			f_length = (f_length + SECTOR_SIZE -1)/SECTOR_SIZE * SECTOR_SIZE;
		}
	}

	/* output file */
	if (d_out) {
		fout = fopen(d_out, "w+");
		if (!fout) {
       		printf("Fail, output open %s, %s\n", d_out, strerror(errno));
			exit(0);
   		}
   		printf("----- save test result to '%s' -----\n", d_out);
   		sync();
	} else {
		fout = stdout;
	}

	/* r/w buffer */
	if (brand) {
		if (BUFFER_MIN_SIZE > buf_min) buf_min = BUFFER_MIN_SIZE;
		if (BUFFER_MAX_SIZE < buf_min) buf_min = BUFFER_MAX_SIZE;
		if (BUFFER_MIN_SIZE > buf_max) buf_max = BUFFER_MIN_SIZE;
		if (BUFFER_MAX_SIZE < buf_max) buf_max = BUFFER_MAX_SIZE;
	} else {
		if (b_length > BUFFER_MAX_SIZE) {
			printf("Fail, Invalid buffer len, max %d byte\n", b_length, BUFFER_MAX_SIZE);
			print_usage();
			exit(0);
		}
	}

	/* file length */
	if (lrand) {
		if (FILE_MIN_SIZE > file_min) file_min = FILE_MIN_SIZE;
		if (FILE_MAX_SIZE < file_min) file_min = FILE_MAX_SIZE;
		if (FILE_MIN_SIZE > file_max) file_max = FILE_MIN_SIZE;
		if (FILE_MAX_SIZE < file_max) file_max = FILE_MAX_SIZE;
	}

	/* real time */
	if (o_rt)
		disk_new_sched(getpid(), SCHED_FIFO, 99);

	/* default operation */
	if (!o_rd && !o_wr)
		o_rd = 1;

	if (!o_sync)
		fflags = 0;

	/* random seed */
	srand(time(NULL));

	/* current disk avail */
	disk_free = disk_disk_avail(path, NULL, 0);

	/* debug message */
	DBG("\n");
	DBG("File path  : '%s'\n", path);
	DBG("File opr   : read (%s), write (%s)\n", o_rd?"yes":"no", o_wr?"yes":"no");

	if (lrand) {
		DBG("File size  : random, min %lld byte, max %lld byte (free disk %lld byte)\n",
			file_min, file_max, disk_free);
	} else {
		DBG("File size  : %8lld byte (free %lld byte)\n", f_length, disk_free);
	}

	if (brand) {
		DBG("Buff size  : random, min %d byte, max %d byte\n", buf_min, buf_max);
	} else {
		DBG("Buff size  : %8d byte       \n", b_length);
	}

	DBG("sync access: %s             \n", o_sync?"yes":"no");
	DBG("check time : %s             \n", o_time?"yes":"no");
	DBG("Test count : %d             \n", run_count);
	DBG("Test loop  : %s        	    \n", o_loop?"yes":" no");
	DBG("\n");

	do {
		for (index = 0; index < run_count; index++) {

			long long length = 0, readl = 0, writel = 0;
			u64 time = 0, *ptime = o_time ? &time : NULL;
			int buffer = 0;
			struct tms ts;
			struct tm *tm;
			clock_t clk;

			if (brand)
				RAND_SIZE(buf_min , buf_max , SECTOR_SIZE, b_length);
			if (lrand)
				RAND_SIZE(file_min, file_max, file_min, f_length);

			clk = times(&ts);
			tm  = localtime(&clk);
			DBG("Test time  : %d-%d-%d %d:%d:%d\n",
				tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

			sprintf(file, "%s/%s.%d.txt", path, FILE_PREFIX, index);
			DBG("Test path  : %s, count[%4d.%4d] \n", file, loop_count, index);

			if (o_wr) {
				/* check disk free */
				disk_free = disk_disk_avail(path, NULL, 0);
				if (f_length > disk_free) {
					ret = disk_obtain_space(path, run_count, f_length);
					if (0 > ret) {
						DBG("No space left on device, free %lld, req %lld\n",
							disk_disk_avail(path, NULL, 0), f_length);
						exit(0);
					}
				}

				/*
				 * write test
				 */
				writel = disk_write(file, fflags, f_length, b_length, &length, &buffer, ptime, 1, o_verify);
				if (0 > writel) {
					printf("Fail write file, length %lld (%lld)\n", writel, length);
					exit(0);
				}
				DBG("Test write : %3lld.%06lld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
					time?_SE_(time):0, time?_US_(time):0, length, buffer,
					time?_MB_PER_S_(writel, time):0, time?_MB_PER_U_(writel, time):0);
			}

			if (o_rd) {
				/* check exist file */
				if (0 > disk_get_info(file, NULL, NULL)) {
					disk_free = disk_disk_avail(path, NULL, 0);
					if (f_length > disk_free) {
						ret = disk_obtain_space(path, run_count, f_length);
						if (0 > ret) {
							DBG("No space left on device, free %lld, req %lld \n",
								disk_disk_avail(path, NULL, 0), f_length);
							exit(0);
						}
					}

					writel = disk_write(file, fflags, f_length, b_length, NULL, NULL, NULL, 0, o_verify);
					if (0 > writel){
						printf("Fail write file to read, length %lld (%lld)\n", writel, length);
						exit(0);
					}
				}

				/*
				 * read test
				 */
				readl = disk_read(file, fflags, f_length, b_length, &length, &buffer, ptime, o_verify);
				if (0 > readl) {
					printf("Fail read length %lld (%lld)\n", readl, length);
					exit(0);
				}
				DBG("Test read  : %3lld.%06lld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
					time?_SE_(time):0, time?_US_(time):0, length, buffer,
					time?_MB_PER_S_(readl, time):0, time?_MB_PER_U_(readl, time):0);
			}
			DBG("\n");
		}
		loop_count++;

	} while (o_loop);

	if (fout)
		fclose(fout);

	return 0;
}
