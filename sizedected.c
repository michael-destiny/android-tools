#include <stdio.h>
#define PRINT(z, x)  printf("sizeof(%s)=%ld\n",z, x);
int main(){
	int a = 1;
	//printf("sizeof(%s) = %ld\n", "char", sizeof(int));
	PRINT("char", sizeof(char));
	PRINT("double", sizeof(double));
	PRINT("float", sizeof(float));
	PRINT("int", sizeof(int));
	PRINT("short", sizeof(short));
	PRINT("long", sizeof(long));
	PRINT("long long", sizeof(long long));
} 
