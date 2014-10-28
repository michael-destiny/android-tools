#include "apkparse.h"
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include <stdio.h>

int 
pulldexfromzip(char *fileName, char *outputDir)
{
	/*
	int fd = open(filename, O_RDONLY);
	if ( fd > 0) {
		gzFile f = gzopen(filename, "rb");
		close(fd);
	}
	else {
		fprintf(stderr, "%s open error", filename);
	}
	*/
	openZip(fileName, outputDir);

	return 0;
}

void
usage(void) {
	fprintf(stderr, "reverse tool use error\n\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "I don't know now...should be: tool xx.apk outputPath\n\n");
}

int
main(int argc, char * argv[])
{
	if (argc < 3) {
		usage();
		return -1;
	}
	int i;
	for( i = 0; i < argc; i++) {
		printf("argv[%d]: %s\n", i, argv[i]);
	}
	int code = pulldexfromzip(*(argv + argc -2), *(argv + argc -1));
	//printf("test compile\n");
	return 0;
}


