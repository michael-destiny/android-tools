bat: main.o 
	rm -rf tmp
	gcc -lz -o parseTool main.o zipparser.o

main.o : main.c zipparser.o apkparse.h
	gcc -c main.c

zipparser.o : zipparser.c apkparse.h
	gcc -c zipparser.c

clean: 
	rm parseTool main.o
	rm -rf tmp

clear:
	rm -rf tmp
