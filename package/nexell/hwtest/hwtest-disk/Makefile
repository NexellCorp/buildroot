all: disk_test 

devfreq: disk_test.o
	$(CC) $(CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o disk_test 

.PHONY: all clean
