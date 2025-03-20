
EXEC = nwsh
OBJS = nwsh.o

all: $(EXEC)

# default 4K stack is not enough; glob->readdir destroyes .bss/.data
$(EXEC): $(OBJS)
	FLTFLAGS="$(FLTFLAGS) -s 8192" $(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	#$(ROMFSINST) /bin/$(EXEC)
	$(ROMFSINST) $(EXEC) /bin/sh

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

