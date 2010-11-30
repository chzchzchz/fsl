NUM_JOBS=6
MAKECMD=make -j$(NUM_JOBS)
CFLAGS=-g -O3
#CFLAGS=-g
export CFLAGS

all: code tools tests draw

clean: code-clean tests-clean

draw: draw-hits draw-scans

draw-hits:
	util/draw_all_hits.sh

draw-scans:
	util/draw_all_scans.sh

code:
	cd src && $(MAKECMD) && cd ..

tools: code
	cd src/tool && $(MAKECMD) && cd ../..

libs:
	make -C lib all

code-clean:
	cd src && make clean && cd ..

tests: code tools
	tests/do_all_tests.sh

tests-extra:
	TEST_CONFIG="EXTRA" tests/do_all_tests.sh

tests-clean:
	rm -f tests/scantool-*/*

libs-clean:
	make -C lib clean
