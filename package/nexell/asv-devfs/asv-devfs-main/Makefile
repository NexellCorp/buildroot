CXXFLAGS += -std=c++11
LDLIBS += -ljson-c
LDLIBS += -lpthread

all: devfreq

OBJS := main.o task.o devfs.o asv_command.o

devfreq: $(OBJS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o devfreq

.PHONY: all clean
