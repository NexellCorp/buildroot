#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <asm/types.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "rawsend.h"

struct raw_result client_result;
int sock_fd;
unsigned char *out_buff;
unsigned char *in_buff;
unsigned char snd_mac[6];
char *src_ifname;
char *dst_ifname;
long total_sent_packets;
long total_recv_packets;
long total_sent_bytes;
int running, waiting;
int success;

void print_result(struct raw_result *result)
{
	double diff, rate;

	diff = result->useconds;
	diff /= 1000000;
	diff += result->seconds;

	rate = 8*(double)result->bytes/diff/1024;

	DBGOUT(" time:    %3u.%02u\n", result->seconds, result->useconds/10000);
	DBGOUT(" packets: %6u\n", result->packets);
	DBGOUT(" bytes:   %6u\n", result->bytes);
	DBGOUT(" rate:    %6.2f\n", rate);
	DBGOUT(" loss:    %6.2f\n", 1 -
	       (double)result->packets / (double)(client_result.packets));
	DBGOUT(" dups:    %6u\n", result->duplicates);
}

void print_server_result(struct raw_result *result)
{
	result->seconds = ntohl(result->seconds);
	result->useconds = ntohl(result->useconds);
	result->packets = ntohl(result->packets);
	result->bytes = ntohl(result->bytes);
	result->duplicates = ntohl(result->duplicates);

	DBGOUT("server report:\n");
	print_result(result);
}

int open_socket(const char *ifname)
{
	struct ifreq ifr;
	int sock;

	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0)
		exit(EXIT_FAILURE);

	/* set promiscuous mode */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ioctl(sock, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sock, SIOCGIFFLAGS, &ifr);

	return sock;
}

void close_socket(int sock)
{
	struct ifreq ifr;

	/* reset promiscuous mode */
	strncpy(ifr.ifr_name, src_ifname, IFNAMSIZ);
	ioctl(sock_fd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags &= ~IFF_PROMISC;
	ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
	shutdown(sock_fd, SHUT_RD);
	close(sock_fd);
}

void print_mac(const char *addr)
{
	int i;

	for (i = 0; i < ETH_ALEN - 1; i++)
		DBGOUT("%02hhx:", addr[i]);
	DBGOUT("%02hhx\n", addr[ETH_ALEN - 1]);
}

int interface_index(int sock, char *ifname)
{
	struct ifreq ifr;
	int i;

	/* retrieve source ethernet interface index */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) < 0)
		return -EXIT_FAILURE;

	return ifr.ifr_ifindex;
}

int interface_addr(int sock, char *ifname, char *addr)
{
	struct ifreq ifr;

	/* retrieve corresponding source MAC */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sock_fd, SIOCGIFHWADDR, &ifr) < 0)
		return -EXIT_FAILURE;
	memcpy(addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	DBGOUT("%02x %02x %02x %02x %02x %02x\n",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	return EXIT_SUCCESS;
}

void *recv_packets_thread(void *ptr)
{
	struct ethhdr *in_hdr;
	char *addr = ptr;
	int i, recv;

	in_hdr = (struct ethhdr *)in_buff;

	while (waiting) {
		recv = recvfrom(sock_fd, in_buff, ETH_FRAME_LEN,
			MSG_DONTWAIT, NULL, NULL);
		if (recv == -1)
			continue;

		if (recv < sizeof(*in_hdr))
			continue;

		/* do not consider packets with wrong destination */
		if (memcmp((const void *)in_hdr->h_dest, addr, ETH_ALEN) != 0)
			continue;
		else
			success++;
	}

	return NULL;
}

void sigint(int signum)
{
	if (!running)
		exit(EXIT_FAILURE);

	running = 0;
}

void sigalarm(int signum)
{
	if (!running)
		exit(EXIT_FAILURE);

	running = 0;
}

double timeval_subtract(struct timeval *result, struct timeval *t2,
			struct timeval *t1)
{
	double ret;
	long int diff;

	diff = (t2->tv_usec + 1000000 * t2->tv_sec)
		- (t1->tv_usec + 1000000 * t1->tv_sec);
	result->tv_sec = diff / 1000000;
	result->tv_usec = diff % 1000000;

	ret = diff;
	ret /= 1000000.0;

	return ret;
}

/* calc interval in microseconds */
int calc_interval(int rate)
{
	return 1e6 * 2 / rate;
}

int main(int argc, char *argv[])
{
	pthread_t rcv_thread;

	out_buff = (void *)malloc(ETH_FRAME_LEN);
	in_buff = (void *)malloc(ETH_FRAME_LEN);
	unsigned char *data_ptr = out_buff + ETH_HLEN;
	unsigned char src_addr[ETH_ALEN], dst_addr[ETH_ALEN];
	struct ethhdr *out_hdr = (struct ethhdr *)out_buff;
	struct sockaddr_ll s_addr;
	struct timeval begin, end, elapsed;
	struct raw_result *svr_result = (struct raw_result *)
		(in_buff + ETH_HLEN + sizeof(END_OF_STREAM));
	double diff;
	int i, sent, recv, rate, interval, timeout = 0, src_idx, dst_idx;

	sock_fd = 0;
	total_sent_packets = 0;
	total_recv_packets = 0;
	total_sent_bytes = 0;
	running = 1;
	waiting = 1;
	success = 0;

	usleep(1000000);

	if (argc < 4) {
		printf("Missing arguments.\n");
		printf("%s [src_ifname] [dest_ifname] ");
		printf("[kbit/s] [seconds]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	src_ifname = argv[1];
	dst_ifname = argv[2];
	rate = atoi(argv[3]);
	interval = calc_interval(rate);
	if (argc == 5)
		timeout = atoi(argv[4]);

	/* fill ethernet payload with some data */
	memset(out_buff, 0, ETH_FRAME_LEN);
	memset(in_buff, 0, ETH_FRAME_LEN);
	memset(&client_result, 0, sizeof(client_result));

	/* open raw socket */
	sock_fd = open_socket(src_ifname);
	if (sock_fd < 0)
		exit(EXIT_FAILURE);

	/* prepare source interface */
	src_idx = interface_index(sock_fd, src_ifname);
	if (src_idx < 0)
		exit(EXIT_FAILURE);

	if (interface_addr(sock_fd, src_ifname, src_addr) < 0)
		exit(EXIT_FAILURE);

	/* prepare destination interface */
	dst_idx = interface_index(sock_fd, dst_ifname);
	if (dst_idx < 0)
		exit(EXIT_FAILURE);

	if (interface_addr(sock_fd, dst_ifname, dst_addr) < 0)
		exit(EXIT_FAILURE);

	/* prepare sockaddr_ll */
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sll_family   = AF_PACKET;
	s_addr.sll_protocol = htons(ETH_P_ALL);
	s_addr.sll_ifindex  = src_idx;
	s_addr.sll_halen    = ETH_ALEN;
	memcpy(s_addr.sll_addr, dst_addr, ETH_ALEN);

	/* bind to interface */
	if (bind(sock_fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
		exit(EXIT_FAILURE);

	/* enable signals */
	signal(SIGINT, sigint);
	signal(SIGALRM, sigalarm);

	if (timeout)
		alarm(timeout);

	/* prepare ethernet header */
	memcpy(out_hdr->h_dest, dst_addr, ETH_ALEN);
	memcpy(out_hdr->h_source, src_addr, ETH_ALEN);
	out_hdr->h_proto = htons(ETH_DATA_LEN);

	pthread_create(&rcv_thread, NULL, recv_packets_thread, src_addr);

	/* record timestamp */
	if (gettimeofday(&begin, 0) < 0)
		exit(EXIT_FAILURE);

	/* prepare counters */
	client_result.sequence = 1;

	while (running) {
		*(int *)data_ptr = htonl(client_result.sequence++);

		sent = sendto(sock_fd, out_buff, ETH_FRAME_LEN, 0,
			(struct sockaddr *)&s_addr, sizeof(s_addr));

		client_result.packets++;
		client_result.bytes += sent;
		usleep(interval);
	}

	/* record timestamp */
	if (gettimeofday(&end, 0) < 0)
		exit(EXIT_FAILURE);

	/* prepare and print result */
	diff = timeval_subtract(&elapsed, &end, &begin);
	client_result.seconds = elapsed.tv_sec;
	client_result.useconds = elapsed.tv_usec;
	DBGOUT("client report:\n");
	print_result(&client_result);

	/* send final packets */
	memcpy(data_ptr, END_OF_STREAM, sizeof(END_OF_STREAM));
	DBGOUT("client snd EOS\n");
	for (i = 0; i < 10; i++)
		sendto(sock_fd, out_buff, ETH_HLEN + sizeof(END_OF_STREAM), 0,
			(struct sockaddr *)&s_addr, sizeof(s_addr));

	waiting = 0;
	/* wait 1 seconds for server result */
	/*
	usleep(1500000);
	waiting = 0;
	usleep(1000000);
	printf("ahn\n");
	recvfrom(sock_fd, in_buff, ETH_DATA_LEN, MSG_DONTWAIT, NULL, NULL);

	DBGOUT("server result :\n");
	print_result((struct raw_result *)(data_ptr+sizeof(END_OF_STREAM)));
	*/

	/* wait for the second thread to finish */
	if (pthread_join(rcv_thread, NULL))
		DBGOUT("Error joining thread\n");

	close_socket(sock_fd);

	free(out_buff);
	free(in_buff);

	printf("Ethernet loopback test : ");
	if (success >= 100) {
		printf("Success!!! (%d)\n", success);
		return EXIT_SUCCESS;
	}

	printf("FAILURE!!! (%d)\n", success);
	return EXIT_FAILURE;
}
