all: postmark

clean:
	rm -f *.o postmark

postmark: postmark.o
	gcc  $< -o $@

postmark.o: postmark-1.51.c
	gcc -O3 -c $< -o $@