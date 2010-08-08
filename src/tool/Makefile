all: scantool browser

clean:
	rm  -f *.s *.o *.bc *.ll fsl.table.c scantool browser

browser: fsl.table.o fsl.types.o
	gcc -O3 -o browser $^ ../browser.o ../runtime.o 
scantool: fsl.table.o fsl.types.o
	gcc -O3 -o scantool $^ ../scantool.o ../runtime.o 
fsl.types.o: fsl.types.s
	as fsl.types.s -o fsl.types.o

fsl.types.s: fsl.types.bc
	llc -O3 fsl.types.bc -o fsl.types.s

fsl.types.bc: fsl.types.ll
	opt -O3 fsl.types.ll >fsl.types.bc

fsl.types.ll:
	../lang <../../fs/ext2.fsl
fsl.table.c:
	../lang <../../fs/ext2.fsl

fsl.table.o: fsl.table.c
	gcc -O3 -c fsl.table.c -o fsl.table.o

