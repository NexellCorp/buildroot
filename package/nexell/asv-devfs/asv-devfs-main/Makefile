CXXFLAGS += -std=c++11
LDLIBS += -ljson-c
LDLIBS += -lpthread

all: devfreq

devfreq: main.o task.o asv.o devfs.o
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o devfreq

.PHONY: all clean
