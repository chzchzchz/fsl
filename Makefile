NUM_JOBS=6
MAKECMD=make -j$(NUM_JOBS)

all: code tools tests draw

clean: clean-code clean-tests

draw: draw-hitmaps draw-scans

draw-hitmaps:
	util/draw_all_hits.sh

draw-scans:
	util/draw_all_scans.sh

code:
	cd src && $(MAKECMD) && cd ..

tools:
	cd src/tool && $(MAKECMD) && cd ../..

clean-code:
	cd src && make clean && cd ..

tests: code tools
	tests/do_all_tests.sh

tests-extra:
	TEST_CONFIG="EXTRA" tests/do_all_tests.sh

clean-tests:
	rm -f tests/scantool-*/*
