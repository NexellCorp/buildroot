/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "task.h"
#include "asv_command.h"
#include "devfs.h"

#define ENABLE_UMS	1

#define MAX_TX_STRING	1024
#define	RET_FILE_NAME	"RESULT.txt"
#define	TTY_CONSOLE	"/dev/ttyS0"
#define	VDD_CORE_ID	4
#define	PWM_PERIOD_HZ	10000000

typedef struct option_t {
	char file[256]  = { 0, };
	char retdir[256]  = { 0, };
	int log_buffsize = 262144;
	bool log_date = true;
	bool log_print = false;
	bool log_redir = true;
	bool log_redir_err = false;
	bool show_task = false;
	bool test_mode = false;
	int task_active[256] = { 0, };
	int active_size = 0;
	int continue_stat = -1;
} OP_T;

typedef struct log_redirect_t {
	char file[256]  = { 0, };
	int fd = -1, logout, logerr;
	bool redirect_err = false;
} LOG_RE_T;

TaskManager *g_manager;

#if 0
#define	LOGMESG(l, t, format, ...) do { \
		fprintf(stdout, format, ##__VA_ARGS__); \
	} while (0)
#define	LOGERR(l, t, format, ...) do { \
		fprintf(stderr, format, ##__VA_ARGS__); \
	} while (0)
#define	LOGDUMP(l, t)	do { } while (0)
#define	LOGDONE(l, t)	do { } while (0)
#else
#define	LOGMESG(l, t, format, ...) do { \
		l->Write(t, format, ##__VA_ARGS__); \
	} while (0)
#define	LOGERR(l, t, format, ...) do { \
		LOGMESG(Log, task, COLOR_RED); \
		l->Write(t, format, ##__VA_ARGS__); \
		LOGMESG(Log, task, COLOR_RESET); \
	} while (0)
#define	LOGDUMP(l, t)	 do { l->Dump(t); } while (0)
#define	LOGDONE(l, t)	 do { l->Done(t); } while (0)
#endif

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

static int mkpath(const char *s, mode_t mode)
{
        char *q, *r = NULL, *path = NULL, *up = NULL;
        int ret = -1;

        if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
                return 0;

        if (!(path = strdup(s)))
                return -errno;

        if (!(q = strdup(s)))
                goto out;

        if (!(r = (char *)dirname(q)))
                goto out;

        if (!(up = strdup(r)))
                goto out;

        if ((mkpath(up, mode) == -1) && (errno != EEXIST))
                goto out;

        if ((mkdir(path, mode) == -1) && (errno != EEXIST))
                ret = -1;
        else
                ret = 0;
out:
        if (up != NULL)
                free(up);

        free(q);
        free(path);

        return ret;
}

static LOG_RE_T *logo_new(const char *dir, const char *logfile,
			bool redirect_err)
{
	LOG_RE_T *r_log;
	char file[256];
	int fd, out = -1, err = -1, ret;

	if (!logfile)
		return NULL;

	if (dir && 0 != access(dir, W_OK)) {
		LogE("%s : %s\n", dir, strerror(errno));
		return NULL;
	}

	if (dir && strlen(dir)) {
		string s = dir;

		if (dir[strlen(dir)-1] != '/')
			s += '/';

		s += logfile;
		strcpy(file, s.c_str());
	} else {
		strcpy(file, logfile);
	}

	fd = open(file, O_RDWR | O_TRUNC | O_CREAT, 0644);
	if (fd < 0)
		return NULL;

	out = dup(STDOUT_FILENO);
	ret = dup2(fd, STDOUT_FILENO);
	if (ret < 0) {
		LogE("%s stdout dup2 %s !!!\n", file, strerror(errno));
		close(out), close(fd);
		return NULL;
	}

	if (!redirect_err)
		goto _end_redirect;

	err = dup(STDERR_FILENO);
	ret = dup2(fd, STDERR_FILENO);
	if (ret < 0) {
		LogE("%s stderr dup2 %s !!!\n", file, strerror(errno));
		close(err);
	}

_end_redirect:
	r_log = new LOG_RE_T;

	strcpy(r_log->file, file);

	r_log->fd = fd;
	r_log->logout = out;
	r_log->logerr = err;

	return r_log;
}

static void logo_del(LOG_RE_T *r_log)
{
	if (!r_log)
		return;

	if (r_log->fd < 0)
		return;

	dup2(r_log->logout, STDOUT_FILENO);
	if (r_log->redirect_err)
		dup2(r_log->logerr, STDOUT_FILENO);

	close(r_log->fd);
	close(r_log->logout);

	if (r_log->redirect_err)
		close(r_log->logerr);

	delete r_log;
}

static int task_command(const char *exec, bool syscmd)
{
	FILE *fp;
	char buf[16];
	size_t len;

	/* check success or failure with exit code or return 0/-ERR */
	if (syscmd) {
		int ret = system(exec);

		return (int)((char)WEXITSTATUS(ret));
	}

	/*
	 * check success or failure with return value range
	 * eg>
	 * ---------------------------------------------------------------------
	 * !/bin/sh
	 * echo `grep processor /proc/cpuinfo | wc -l`
	 * ---------------------------------------------------------------------
	 * ---> return "cpu number"
	 */
	fp = popen(exec, "r");
	if (!fp)
		return errno;

	len = fread((void*)buf, sizeof(char), sizeof(buf), fp);
	if (!len) {
		pclose(fp);
	        return errno;
	}
	pclose(fp);

	return strtol(buf, NULL, sizeof(buf));
}

static int task_execute(TASK_DESC_T *task)
{
	TaskManager *manager = task->manager;
	OP_T *op = static_cast<OP_T *>(manager->GetData());
	string s;
	const char *c = task->path;
	TIMESTEMP_T *time = &task->time;
	char buff[1024];
	long long ts = 0, td = 0;
	bool syscmd = false;
	bool success = true;
	int loop = 1, ret;
	LOG_RE_T * r_log = NULL;
	Logger *Log = manager->GetLogger();

	if (!task->exec)
		return -EINVAL;

	LOGMESG(Log, task, "------------------------------------------------------------------------\n");
	LOGMESG(Log, task, "ID       : [%d] %d (%d) \n", task->priority, task->id, getpid());
	LOGMESG(Log, task, "DESC     : %s\n", task->desc);
	LOGMESG(Log, task, "EXEC     : %s\n", task->exec);
	LOGMESG(Log, task, "THREAD   : %s\n", task->isthread ? "Y" : "N");
	LOGMESG(Log, task, "LOGFILE  : %s\n", task->logo);
	LOGMESG(Log, task, "ACT      : loop:%d/%d, delay:%d ms, sleep:%d ms\n",
		task->count, task->loop, task->delay, task->sleep);
	LOGMESG(Log, task, "------------------------------------------------------------------------\n");

	if (c) {
		s = c;
		if (c[strlen(c)-1] != '/')
			s += '/';
	}

	strcpy(buff, task->cmd);
	c = strtok(buff, " ");
	s += c;

	if (0 != access(s.c_str(), X_OK)) {
		task->retval = errno;
		LOGERR(Log, task, "ERROR    : Failed check access [%s]\n", s.c_str());
		goto _err_execute;
	}

	if (task->min == 0 && task->max == 0)
		syscmd = true;

	if (task->delay)
		msleep(task->delay);

	if (task->loop)
		loop = task->loop;

	if (manager->TaskFailed())
		goto _err_execute;

	/* Log redirect */
	if (op->log_redir)
		r_log = logo_new(manager->GetResultDir(),
					task->logo, op->log_redir_err);

	task->pid = getpid();
	task->running = true;

	for (int i = 0; i < loop; i++) {
		RUN_TIMESTAMP_US(ts);

		ret = task_command(task->exec, syscmd);

		task->retval = ret;
		task->count++;

		END_TIMESTAMP_US(ts, td);
		SET_TIME_STAT(time, td);

		if (ret < task->min || ret > task->max) {
			if (!manager->IsContinue()) {
				manager->SetFail();
				break;
			}

			task->success = false;
			success = false;
		}

		if (success)
			task->success = true;

		/* SetFail other task */
		if (manager->TaskFailed())
			break;

		if (task->sleep)
			msleep(task->sleep);
	}

	/* Close Log redirect */
	logo_del(r_log);

_err_execute:
	task->running = false;

	LOGMESG(Log, task, task->success ? COLOR_GREEN : COLOR_RED);
	LOGMESG(Log, task, "------------------------------------------------------------------------\n");
	if (time->cnt)
		LOGMESG(Log, task, "TIME     : min:%2llu.%03llu ms, max:%2llu.%03llu ms, avr:%2llu.%03llu ms\n",
			time->min/1000, time->min%1000, time->max/1000, time->max%1000,
			(time->tot/time->cnt)/1000, (time->tot/time->cnt)%1000);

	LOGMESG(Log, task, "RESULT   : [ID:%d, %d] %s, RET:%d [%d,%d] %s/%s \n",
		task->id, task->priority,
		!task->count ? "NO RUN" : task->success ? "OK" : "FAIL",
		task->retval, task->min, task->max,
		task->exec, task->success ? "" : strerror(task->retval));
	LOGMESG(Log, task, "------------------------------------------------------------------------\n");
	LOGMESG(Log, task, COLOR_RESET);
	LOGDUMP(Log, task); LOGDONE(Log, task);

	return task->success ? 0 : -1;
}

static void *thread_execute(void *data)
{
	TASK_DESC_T *task = static_cast<TASK_DESC_T *>(data);
	TaskManager *manager = task->manager;
	int pid, status;

	pid = fork();
	if (pid == 0) {
		int ret = task_execute(task);
		exit(ret);
	}

	while (1) {
		task->pid = pid; /* chield task pid */

		if (waitpid(pid, &status, WNOHANG))
			break;

		if (!manager->IsContinue() && manager->TaskFailed()) {
			kill(pid, SIGTERM);
			waitpid(pid, &status, 0);
			break;
		}

		msleep(100);
	}

	if (!status)
		task->success = true;

	if (!manager->IsContinue() && status)
		manager->SetFail();

	pthread_exit(NULL);
}

static int run_execute(TaskManager *manager)
{
	OP_T *op = static_cast<OP_T *>(manager->GetData());
	vector <pthread_t> handle;
	const char *retdir = manager->GetResultDir();
	char dir[256];

	if (strlen(op->retdir))
		retdir = op->retdir;

	/* create result directory */
	if (retdir) {
		if (op->log_date) {
  			time_t timer = time(NULL);
  			struct tm *t = localtime(&timer);

  			sprintf(dir, "%s/%d%02d%02d-%02d%02d",
  				retdir, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
  				t->tm_hour, t->tm_min);
  			retdir = dir;
		}
		mkpath(retdir, 0755);
	}

	manager->SetResultDir(retdir);

	/* create Diagnostic Log */
	Logger *Log = new Logger(retdir, RET_FILE_NAME,
				op->log_buffsize, op->log_print);
	manager->SetLogger(Log);

	LOGMESG(Log, NULL, "========================================================================\n");
	LOGMESG(Log, NULL, "RETDIR   : %s\n", manager->GetResultDir());
	LOGMESG(Log, NULL, "CONTINUE : %s\n", manager->IsContinue() ? "Y" : "N");
	LOGMESG(Log, NULL, "========================================================================\n");
	LOGDUMP(Log, NULL); LOGDONE(Log, NULL);

	for (int i = 0 ; i < manager->GetTaskNum(); i++) {
		TASK_DESC_T *task = manager->TaskGet(i);

		if (op->active_size) {
			for (int n = 0; n < op->active_size; n++) {
				if (op->task_active[n] == task->priority) {
					task->active = true;
					break;
				}
			}

			if (!task->active)
				continue;
		}

		task->active = true;
		task->manager = manager;

		if (task->isthread) {
			pthread_t hnd;
			if (pthread_create(&hnd, NULL, thread_execute, task)) {
				LogE("%s : %s !!!\n",
					task->exec, strerror(errno));
				if (!manager->IsContinue())
					return -1;
			}
			handle.push_back(hnd);
			continue;
		}

		int ret = task_execute(task);

		if (ret || manager->TaskFailed())
			return -1;
	}

	for (auto ls = handle.begin(); ls != handle.end();) {
		pthread_t hnd = (*ls);

		pthread_join(hnd, NULL);
		ls = handle.erase(ls);
	}

 	if (manager->TaskFailed())
 		return -1;

	return 0;
}

static void signal_handler(int sig)
{
	TaskManager *manager = g_manager;

	/* kill running tasks */
	for (int i = 0 ; i < manager->GetTaskNum(); i++) {
		TASK_DESC_T *task = manager->TaskGet(i);

		if (task && task->pid) {
			int status;
			pid_t ret;

			ret = waitpid(task->pid, &status, WNOHANG);

			/* Child still alive */
			if (!ret)
 				kill(task->pid, SIGTERM);
		}
	}

	exit(EXIT_FAILURE);
}

static void signal_register(void)
{
	struct sigaction sact;

	sact.sa_handler = signal_handler;

  	sigemptyset(&sact.sa_mask);

  	sact.sa_flags = 0;

	sigaction(SIGINT , &sact, 0);
	sigaction(SIGTERM, &sact, 0);
}

static void print_help(const char *name, OP_T *op)
{
	LogI("\n");
	LogI("usage: options\n");
	LogI("\t-f : config file path\n");
	LogI("\t-r : set result directory, log is exist 'result/<date>', refer to '-d'\n");
	LogI("\t-d : skip date directory in result directory\n");
	LogI("\t-l : log buffer size for each task print log default[%d]\n", op->log_buffsize);
	LogI("\t-n : no redirect application's message\n");
	LogI("\t-p : enable diagnostic's message print immediately\n");
	LogI("\t-e : redirect application's 'STDERR' printf err\n");
	LogI("\t-i : show tasks in config file\n");
	LogI("\t-a : set active task numbers with priority ex> 1,2,3,...\n");
	LogI("\t-c : set application running status if 'y' continue, if 'n' stop when failed\n");
	LogI("\t-t : test mode (not devfs)\n");
	LogI("\n");
	LogI("\tcheck json grammar : cat <file> | python -m json.tool\n");
}

static OP_T *parse_options(int argc, char **argv)
{
	int opt;
	OP_T *op = new OP_T;

	while (-1 != (opt = getopt(argc, argv, "hf:r:nl:edpa:ic:t"))) {
		switch(opt) {
       		case 'f':
       			strcpy(op->file, optarg);
       			break;
       		case 'r':
       			strcpy(op->retdir, optarg);
       			break;
       		case 'd':
       			op->log_date = false;
       			break;
       		case 'l':
       			op->log_buffsize = strtoul(optarg, NULL, 10);
       			break;
       		case 'e':
       			op->log_redir_err = true;
       			break;
       		case 'n':
       			op->log_redir = false;
       			break;
       		case 'p':
       			op->log_print = true;
       			break;
       		case 'i':
       			op->show_task = true;
       			break;
       		case 'a':
       			{
			int size = 0;
       			char *s = optarg, *c = strtok(s, " ,.-");

			while (c != NULL) {
				int id = strtoul(c, NULL, 10);

				c = strtok(NULL, " ,.-");
				op->task_active[size++] = id;

				if ((int)ARRAY_SIZE(op->task_active) < size) {
					LogE("over task array size[%d] !!!\n",
						(int)ARRAY_SIZE(op->task_active));
					exit(EXIT_FAILURE);
				}
				}
				op->active_size = size;
				}
				break;
			case 'c':
				{
				char *s = optarg;

				if (!strcmp(s, "y"))
					op->continue_stat = 1;

				if (!strcmp(s, "n"))
					op->continue_stat = 0;
				}
				break;
			case 't':
			op->test_mode = true;
			break;
			default:
				print_help(argv[0], op);
				exit(EXIT_FAILURE);
				break;
			}
	}

	return op;
}

static int run_tasks(OP_T *op)
{
	TaskManager *manager = new TaskManager;

	g_manager = manager;

	if (!manager->LoadTask(op->file))
		exit(EXIT_FAILURE);

	if (op->continue_stat != -1)
		manager->SetContinue(op->continue_stat ? true : false);

	if (op->show_task) {
		for (int i = 0 ; i < manager->GetTaskNum(); i++) {
			TASK_DESC_T *task = manager->TaskGet(i);

			LogI("------------------------------------------------------------------------\n");
			LogI("ID       : [%d] %d \n", task->priority, task->id);
			LogI("DESC     : %s\n", task->desc);
			LogI("EXEC     : %s\n", task->exec);
			LogI("THREAD   : %s\n", task->isthread ? "Y" : "N");
			LogI("ACT      : loop:%d/%d, delay:%d ms, sleep:%d ms\n",
				task->count, task->loop, task->delay, task->sleep);
			LogI("LOGFILE  : %s\n", task->logo);
		}

		return 0;
	}

	signal_register();

	manager->SetData(op);

	/*
	 * execute tasks
	 */
	run_execute(manager);

	Logger *Log = manager->GetLogger();
	bool success = manager->IsSuccessAll();

	LOGMESG(Log, NULL, "========================================================================\n");
	if (success)
		LOGMESG(Log, NULL, "\033[42m");
	else
		LOGMESG(Log, NULL, "\033[43m");

	LOGMESG(Log, NULL, "EXIT : %s, LOG: %s\n", success ? "SUCCESS" : "FAIL", manager->GetResultDir());
	LOGMESG(Log, NULL, "\033[0m\r\n");
	LOGMESG(Log, NULL, "========================================================================\n");
	LOGDUMP(Log, NULL); LOGDONE(Log, NULL);

	return success ?  0 : -EFAULT;
}

static void tty_puts(int fd, const char *msg)
{
	int ret;

	fprintf(stdout, msg);

	ret = write(fd, msg, strlen(msg));
	if(ret < 0)
		LogE("Error: tty puts :%s\n", msg);
}

static void ASVMSG(int fd, const char *fmt, ...)
{
	char str[MAX_TX_STRING];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(str, MAX_TX_STRING - 1, fmt, ap);
	va_end(ap);

	tty_puts(fd, str);
}

static int open_asv_console(const char *terminal)
{
	struct termios	newtio, oldtio;
	int fd;

	fd = open(terminal, O_RDWR | O_NOCTTY);
	if (fd < 0)
		return -EINVAL;

	tcgetattr(fd, &oldtio);

	memcpy(&newtio, &oldtio, sizeof(newtio));
	newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD | IXOFF;
	newtio.c_iflag = IGNPAR | ICRNL;

	newtio.c_oflag = 0;
	newtio.c_lflag = ICANON;

	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 10;

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	return fd;
}

#if 0
static int setup_asv_task(OP_T *op)
{
	unsigned int ids[2] = { 0, }, hpm[8] = { 0, };
	char data[32] = { 0, };
	const char *cmd;
	int ret;

	cmd = "ums.configfs.sh lun";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	mkpath("/mnt/ehci0", 0755);
	mkpath("/mnt/ehci1", 0755);
	mkpath("/mnt/mmc0" , 0755);
	mkpath("/mnt/mmc1" , 0755);
	mkpath("/mnt/mmc2" , 0755);

	/*
	 * PWMs
	 */

	/* PWM 0 */
	sprintf(data, "%d", 0);
	sys_write("/sys/class/pwm/pwmchip0/export", data, strlen(data));
	sprintf(data, "%d", PWM_PERIOD_HZ);
	sys_write("/sys/class/pwm/pwmchip0/pwm0/period", data, strlen(data));
	sprintf(data, "%d", 1);
	sys_write("/sys/class/pwm/pwmchip0/pwm0/enable", data, strlen(data));

	/* PWM 1 */
	sprintf(data, "%d", 1);
	sys_write("/sys/class/pwm/pwmchip0/export", data, strlen(data));
	sprintf(data, "%d", PWM_PERIOD_HZ);
	sys_write("/sys/class/pwm/pwmchip0/pwm1/period", data, strlen(data));
	sprintf(data, "%d", 1);
	sys_write("/sys/class/pwm/pwmchip0/pwm1/enable", data, strlen(data));

	/* PWM 2 */
	sprintf(data, "%d", 2);
	sys_write("/sys/class/pwm/pwmchip0/export", data, strlen(data));
	sprintf(data, "%d", PWM_PERIOD_HZ);
	sys_write("/sys/class/pwm/pwmchip0/pwm2/period", data, strlen(data));
	sprintf(data, "%d", 1);
	sys_write("/sys/class/pwm/pwmchip0/pwm2/enable", data, strlen(data));

	/* PWM 3 */
	sprintf(data, "%d", 3);
	sys_write("/sys/class/pwm/pwmchip0/export", data, strlen(data));
	sprintf(data, "%d", PWM_PERIOD_HZ);
	sys_write("/sys/class/pwm/pwmchip0/pwm3/period", data, strlen(data));
	sprintf(data, "%d", 1);
	sys_write("/sys/class/pwm/pwmchip0/pwm3/enable", data, strlen(data));

	/*
	 * Ethernet
	 */
	cmd = "ifconfig eth0 hw ether \"00:50:56:c1:11:02\"";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);

	cmd = "ifconfig eth0 192.168.1.151 up";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);
	
	cmd = "ifconfig eth1 hw ether \"00:50:56:c1:11:03\"";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);

	cmd = "ifconfig eth1 192.168.1.151 up";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);

	/*
	 * Mount
	 */
	/* Mount MMC0 */
	cmd = "mount /dev/mmcblk0p2 /mnt/mmc0";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	/* Mount MMC1 */
	cmd = "mount /dev/mmcblk1p1 /mnt/mmc1";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	/* Mount MMC2 */
	cmd = "mount /dev/mmcblk2p1 /mnt/mmc2";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	/* Mount EHCI1 (usb disk) */
	cmd = "mount /dev/sda1 /mnt/ehci1";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	/* Mount EHCI0 (usb lun) */
	msleep(1500);
	cmd = "mount /dev/sdb /mnt/ehci0";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	/*
	 * Check IDs
	 */
	ret = sys_uid_ids(ids, 2);
	if(ret) {
		LogE("Error: Parsing uuid's ids\n");
		return ret;
	}
	LogI("IDS:%02x-%02x\n", ids[0], ids[1]);

	ret = sys_uid_hpm(hpm, 8);
	if(ret) {
		LogE("Error: Parsing uuid's hpm\n");
		return ret;
	}
	LogI("HPM:%03x-%03x-%03x-%03x-%03x-%03x-%03x-%03x\n",
		hpm[0], hpm[1], hpm[2], hpm[3],
		hpm[4], hpm[5], hpm[6], hpm[7]);

	return 0;
}
#else
static int setup_asv_task(OP_T *op)
{
	const char *cmd;
	int ret;

	//	Usb Mass Storage
	cmd = "ums.configfs.sh lun";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

#if 1
	/*
	 * Ethernet
	 */
	cmd = "ifconfig eth0 hw ether \"00:50:56:c1:11:02\"";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);

	cmd = "ifconfig eth0 192.168.1.151 up";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);

	cmd = "ifconfig eth1 hw ether \"00:50:56:c1:11:03\"";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);

	cmd = "ifconfig eth1 192.168.1.152 up";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);
#if 0
	cmd = "server eth1 &";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
	     /*	return ret; */
	}
	printf("ret.%d %s\n", ret, cmd);
#endif
#endif

	//	FIXME : Waiting usb mass storage mount functionality.
	sleep(2);


	//
	//	Mount OTG USB Mass Storage Driver With USB Host Port
	//
	mkpath("/mnt/otg_ums" , 0755);
	cmd = "mount -t vfat /dev/sdb /mnt/otg_ums";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	//	Mount USB Disk for VPU Test
	mkpath("/mnt/usbdsk" , 0755);
	cmd = "mount /dev/sda1 /mnt/usbdsk";
	ret = system(cmd);
	ret = (int)((char)WEXITSTATUS(ret));
	if (ret) {
		LogE("Error: system call '%s', ret:%d\n", cmd, ret);
		return ret;
	}

	sleep(1);

	return 0;
}
#endif

int main(int argc, char **argv)
{
	OP_T *op;
	int fd;
	ASV_COMMAND cmd;
	ASV_MODULE_ID id;
	ASV_PARAM param;
	long uV;
	int temp, vdd = VDD_CORE_ID;
	unsigned int ids[2] = { 0, }, hpm[8] = { 0, }, ecid[4] = { 0, };
	int ret;

	op = parse_options(argc, argv);
	if (!op)
		exit(EXIT_FAILURE);

	ret = setup_asv_task(op);
	if (ret) {
		LogE("Error: SETUP TASKS !!!\n");
		return ret;
	}

	if (op->test_mode)
		return run_tasks(op);

	fd = open_asv_console(TTY_CONSOLE);
	if (fd < 0)
		return fd;

	ASVMSG(fd, "\nBOOT DONE.\n");

	while (1)
	{
		char buff[128] = { 0, };

		read(fd, buff, sizeof(buff));

		ret = ParseStringToCommand(buff, sizeof(buff), &cmd, &id, &param);
		if (ASV_RES_OK != ret)
		{
			LogE("Error: Parsing command: %s\n", buff);
			continue;
		}

		switch (cmd)
		{
		case ASVC_SET_FREQ:
			ASVMSG(fd, "ASVC_SET_FREQ DEV (id=%d, %s)\n", id, ASVModuleIDToString(id));
			if( (id != ASVM_MM) && (id != ASVM_SYS) ){
				ASVMSG(fd, "FAIL : Not support id:%d\n", id);
				break;
			}
			if( id == ASVM_MM )
			{
				ret = sys_dev_mm_coda_set_freq(param.u32);
			}
			else if( id == ASVM_SYS )
			{
				ret = sys_dev_sysbus_set_freq(param.u32);
			}

			if(ret)
				ASVMSG(fd, "FAIL : ASVC_SET_FREQ %s %dHz\n", ASVModuleIDToString(id), param.u32);
			else
				ASVMSG(fd, "SUCCESS : ASVC_SET_FREQ %s %dHz\n", ASVModuleIDToString(id), param.u32);
			break;

		case ASVC_SET_VOLT:
			uV = param.f64 * 1000000;

			ASVMSG(fd, "ASVC_SET_VOLT DEV (id=%d, %s)\n", id, ASVModuleIDToString(id));
			if( (id != ASVM_MM) && (id != ASVM_SYS) ){
				ASVMSG(fd, "FAIL : Not support id:%d\n", id);
				break;
			}
			if( id == ASVM_MM )
			{
				ret = sys_dev_mm_set_volt( uV );
			}
			else if( id == ASVM_SYS )
			{
				sys_dev_sysbus_set_volt( uV );
			}
			if(ret)
				ASVMSG(fd, "FAIL : ASVC_SET_VOLT %s %fv\n", ASVModuleIDToString(id),
					(float)((uV)/1000000.));
			else
				ASVMSG(fd, "SUCCESS : ASVC_SET_VOLT %s %fv\n", ASVModuleIDToString(id),
					(float)((uV)/1000000.));
			break;

		case ASVC_GET_IDS:
			ret = sys_uid_ids(ids, ARRAY_SIZE(ids));
			if(ret)
				ASVMSG(fd, "FAIL : ASVC_GET_IDS\n");
			else
				ASVMSG(fd, "SUCCESS : IDS=%02x-%02x\n", ids[0], ids[1]);
			break;

		case ASVC_GET_HPM:
			ret = sys_uid_hpm(hpm, ARRAY_SIZE(hpm));
			if(ret)
				ASVMSG(fd, "FAIL : ASVC_GET_HPM\n");
			else
				ASVMSG(fd, "SUCCESS : HPM=%03x-%03x-%03x-%03x-%03x-%03x-%03x-%03x\n",
					hpm[0], hpm[1], hpm[2], hpm[3], hpm[4], hpm[5], hpm[6], hpm[7]);
			break;

		case ASVC_GET_CPUHPM:
		{
			uint32_t hpm;
			GetHPM_CPU( &hpm );
			ASVMSG( fd, "SUCCESS : HPM=%02x\n", hpm);
			break;
		}

		case ASVC_GET_CORE_HPM:
		{
			uint32_t hpm_c;
			GetHPM_CORE( &hpm_c );
			ASVMSG( fd, "SUCCESS : CORE HPM=%02x\n", hpm_c);
			break;
		}

		case ASVC_GET_ECID:
			ret = sys_uid_ecid(ecid, ARRAY_SIZE(ecid));
			if(ret)
				ASVMSG(fd, "FAIL : ASVC_GET_ECID\n");
			else
				ASVMSG(fd, "SUCCESS : ECID=%08x-%08x-%08x-%08x\n",
					ecid[0], ecid[1], ecid[2], ecid[3]);
			break;

		case ASVC_GET_TMU0:
			if(sys_temp_get(0, &temp))
				ASVMSG(fd, "FAIL : ASVC_GET_TMU0\n");
			else
				ASVMSG(fd, "SUCCESS : TMU=%d\n", temp);
			break;

		case ASVC_SET_AXI_FREQ:
			ASVMSG(fd, "ASVC_SET_AXI_FREQ : %d\n", param.u32);
			ret = sys_dev_mm_axi_set_freq( param.u32 );
			if( ret )
				ASVMSG(fd, "FAIL : ASVC_SET_AXI_FREQ %s %dHz\n", ASVModuleIDToString(id), param.u32);
			else
				ASVMSG(fd, "SUCCESS : ASVC_SET_AXI_FREQ %s %dHz\n", ASVModuleIDToString(id), param.u32);
			break;

		case ASVC_RUN:
			ASVMSG(fd, "TEST : RUN TEST(%d)\n", cmd);
			if ( id == ASVM_MM )
			{
				//
				//	just run only VPU
				op->task_active[0] = 0;		//	0 means VPU's task id ==> This is comes form input json file
				op->active_size = 1;
			}

			ret = run_tasks(op);
			if(!ret)
				ASVMSG(fd, "SUCCESS : ASVC_RUN %s\n", ASVModuleIDToString(id));
			else
				ASVMSG(fd, "FAIL : ASVC_RUN %s\n", ASVModuleIDToString(id));
			break;
		default:
			ASVMSG(fd, "FAIL : Unknown Command(%d)\n", cmd);
			break;
		}	// end of switch

		if (ret) {
			LogE("Error: result !!!\n");
			break;
		}
	}


	return ret;
}
