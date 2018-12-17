CXXFLAGS += -std=c++11
LDLIBS += -lpthread 
LDLIBS += -lz

all: cpufreq

cpufreq: asv_command.o dvfsutil.o asv_test.o hwio.o cpu_test.o 
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o cpufreq                                                       
                                                                                 
 .PHONY: all clean                                                               
                     
