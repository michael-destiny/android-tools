bat: main.o 
	rm -rf tmp
	gcc -o parseTool main.c zipparser.c

main.o : main.c zipparser.o
	gcc -c main.c

zipparser.o : zipparser.c
	gcc -c zipparser.c

clean: 
	rm parseTool main.o
	rm -rf tmp

clear:
	rm -rf tmp
