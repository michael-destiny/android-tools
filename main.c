#include "apkparse.h"
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include <stdio.h>

int 
pulldexfromzip(char *fileName)
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
	openZip(fileName);

	return 0;
}

void
usage(void) {
	fprintf(stderr, "reverse tool use error\n\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "I don't know now...should be: tool xx.apk\n\n");
}

int
main(int argc, char * argv[])
{
	if (argc < 2) {
		usage();
		return -1;
	}
	int code = pulldexfromzip(*(argv + argc -1));
	//printf("test compile\n");
	return 0;
}


