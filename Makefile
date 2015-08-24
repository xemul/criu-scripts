OBJS=stopexec ptrace-wait

all: $(OBJS)

ptrace-wait: ptrace-wait.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

stopexec: stopexec.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

.PHONY: clean

clean:
	rm -f $(OBJS)
