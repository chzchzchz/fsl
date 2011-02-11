ifndef CFLAGS
    $(error No CFLAGS. Run from top level.)
endif

ifndef OBJDIR
	$(error No OBJDIR. Run from top level.)
endif

ifndef BINDIR
	$(error No OBJDIR. Run from top level.)
endif

all: kern $(BINDIR)/fslcmd

kern:
	make -C $(LINUX_SRCDIR) M=`pwd`/mod/

$(BINDIR)/fslcmd: $(OBJDIR)/kernel/fslcmd.o
	gcc -o $@ $<


$(OBJDIR)/kernel/%.o: user/%.c
	gcc $(CFLAGS) -I../runtime/ -Imod -c $< -o $@

clean:
	cd mod; make clean; cd ..
