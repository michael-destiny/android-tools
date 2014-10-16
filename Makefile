bat: main.o 
	gcc -o parseTool main.c zipparser.c

main.o : main.c zipparser.o
	gcc -c main.c

zipparser.o : zipparser.c
	gcc -c zipparser.c

clean: 
	rm parseTool main.o
