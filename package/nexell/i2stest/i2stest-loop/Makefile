all: i2s_test

devfreq: i2s_test.o
	$(CC) $(CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o i2s_test

.PHONY: all clean
