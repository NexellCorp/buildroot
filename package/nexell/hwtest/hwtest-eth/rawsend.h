#ifndef RAWSOCKET_H
#define RAWSOCKET_H

#define END_OF_STREAM "thank you for testing\n"

struct raw_result {
	unsigned int seconds;
	unsigned int useconds;
	unsigned int packets;
	unsigned int bytes;
	unsigned int sequence;
	unsigned int duplicates;
};

#if (0)
#define DBGOUT(msg...)		{ printf(msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#endif
