#include "apkparse.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

void openZip(const char *zipFileName, const char *outputDir) {
	//printf("start openZip\n");
	
	int result;
	if(access(zipFileName, F_OK) != 0) {
		fprintf(stderr, "%s is not regular file:%d\n", zipFileName, result);
		return ;	
	}
	
	int zipFile = open(zipFileName, O_RDONLY);
	if (zipFile <= 0) {
		fprintf(stderr, "open file error:%s\n", strerror(errno));
		return ;
	}
	off_t fileLength, seekStart;
	long readAmount;
	//char buf[ZIP_END_OF_CENTEN_DIRECTORY_SEARCH];
	unsigned char *buf = malloc(ZIP_END_OF_CENTEN_DIRECTORY_SEARCH);
	if (NULL == buf) {
		fprintf(stderr, "openZip malloc fail\n");
		goto bail;
	}

	fileLength = lseek(zipFile, 0, SEEK_END);
	if (fileLength < ZIP_END_OF_CENTEN_DIRECTORY) {
		fprintf(stderr, "%ld too small\n", (long)fileLength);
		goto bail;
	}
	//fprintf(stdout, "%s length: %ld\n", zipFileName, (long)fileLength);
	//
#if 0
	lseek(zipFile, 0, SEEK_SET);
	ssize_t read1 = read(zipFile, buf, ZIP_END_OF_CENTEN_DIRECTORY_SEARCH);
	fprintf(stdout, "reads read1%ld\n", read1);

#endif
	
	if (fileLength > ZIP_END_OF_CENTEN_DIRECTORY_SEARCH) {
		//fprintf(stdout, "length > zip_end_of_centen_directory_search\n");
		seekStart = fileLength - ZIP_END_OF_CENTEN_DIRECTORY_SEARCH;
		readAmount = ZIP_END_OF_CENTEN_DIRECTORY_SEARCH;
	}
	else {
		//fprintf(stdout, "length < zip_end_of_centen_directory_search\n");
		seekStart = 0;
		readAmount = (long)fileLength;
	}
	off_t pos = lseek(zipFile, seekStart, SEEK_SET);
	if (pos != seekStart) {
		fprintf(stderr, "seeking centendir error\n");
		goto bail;
	}
	fprintf(stdout, "after seek,current loc:%ld, fileLength: %ld\n", (long)pos, (long)fileLength);
	pos = tell(zipFile);
	fprintf(stdout, "current loc:%ld, fileLength: %ld\n", (long)pos, (long)fileLength);
	//when you read the end of the file, read() would return 0???
	size_t reads = read(zipFile, buf, (size_t)readAmount);
	fprintf(stdout, "reads %ld\n", reads);
	/*
	if (read(zipFile, buf, (size_t)readAmount) != 0) {
		fprintf(stderr, "read error data\n");
		goto bail;
	}
	*/

	int i;
	for (i = readAmount - 4; i >= 0; i--) {
		/*
		if (buf[i] == 0x50) {
			fprintf(stdout, "find 0x50 and value: %ld\n", getLongLE(buf + i));
		}
		*/
		if (buf[i] == 0x50 &&
				getLongLE(buf + i) == ZIP_EOCD_SIGNATURE) {
			fprintf(stdout, "found EOCD at buf + %d\n", i);
			break;
		}
	}

	if (i < 0) {
		fprintf(stderr, "EOCD not found, not Zip\n");
		goto bail;
	}
	off_t EOCD_start = seekStart + i;
	/* extract eocd values */
	fprintf(stdout, "start to read EOCD\n");
	EndOfCentralDir mEOCD;
	int err = readEndOfCentralDir(&mEOCD, buf + i, readAmount - i);
	
	if(mEOCD.mDiskNumber != 0 || mEOCD.mDiskWithCentralDir != 0 ||
			mEOCD.mNumEntries != mEOCD.mTotalNumEntries)
	{
		fprintf(stderr, "Archive spanning not support.\n");
		goto bail;
	}
	pos = lseek(zipFile, mEOCD.mCentralDirOffset, SEEK_SET);
	if (pos != mEOCD.mCentralDirOffset) {
		fprintf(stderr, "try to read mCentralDirOffset\n");
		goto bail;
	}
	
	mEOCD.mEntries = malloc(mEOCD.mTotalNumEntries * sizeof(ZipEntry));
	if(NULL == mEOCD.mEntries) {
		fprintf(stderr, "malloc mEOCD->mEntries fail\n");
		goto bail;
	}
	free(buf);
	buf = malloc(mEOCD.mCentralDirSize);
	if(NULL == buf) {
		fprintf(stderr, "bad malloc in mEOCD.mCentralDirSize\n");
		goto bail;
	}
	reads = read(zipFile, buf, mEOCD.mCentralDirSize);
	if (reads != mEOCD.mCentralDirSize) {
		fprintf(stderr, "read less data in mEOCD.mCentralDirSize\n");
		goto bail;
	}
	/*
	 * I just want to check whether CentralDirEntry was followed by EndOfCentralDir.
	fprintf(stdout, "mCentralDirStart:%ld, size: %ld, endloc:%ld, eocdStart:%ld\n", mEOCD.mCentralDirOffset, mEOCD.mCentralDirSize,
			mEOCD.mCentralDirOffset + mEOCD.mCentralDirSize, (long)EOCD_start);
	*/
	int j;
	for (i = 0, j = 0; i < mEOCD.mTotalNumEntries; i++)  {
		CentralDirEntry *cde = &((*(mEOCD.mEntries + i)).mCDE);
		if(getLongLE(&buf[j]) != ZIP_CDE_SIGNATURE) {
			fprintf(stderr, "read CentralDirEntry signature error.\n");
			goto bail;
		}
		cde->mVersionMadeBy = getShortLE(buf + j + 0x04);
		cde->mVersionToExtract = getShortLE(buf + j + 0x06);
		cde->mGPBitFlag = getShortLE(buf + j + 0x08);
		cde->mCompressionMethod = getShortLE(buf + j + 0x0a);
		cde->mLastModFileTime = getShortLE(buf + j + 0x0c);
		cde->mLastModFileDate = getShortLE(buf + j + 0x0e);
		cde->mCRC32 = getLongLE(buf + j + 0x10);
		cde->mCompressedSize = getLongLE(buf + j + 0x14);
		cde->mUnCompressedSize = getLongLE(buf + j + 0x18);
		cde->mFileNameLength = getShortLE(buf + j + 0x1c);
		cde->mExtraFieldLength = getShortLE(buf + j + 0x1e);
		cde->mFileCommentLength = getShortLE(buf + j + 0x20);
		cde->mDiskNumberStart = getShortLE(buf + j + 0x22);
		cde->mInternalAttrs = getShortLE(buf + j + 0x24);
		cde->mExternalAttrs = getShortLE(buf + j + 0x26);
		cde->mLocalHeaderRelOffset = getLongLE(buf + j + 0x2a);
		int offset = 0x2e;
		if ( cde->mFileNameLength > 0) {
			cde->mFileName = malloc(cde->mFileNameLength + 1);
			memcpy(cde->mFileName, buf + j + offset, cde->mFileNameLength);
			*(cde->mFileName + cde->mFileNameLength)= '\0';
			offset += cde->mFileNameLength;
		}
		if ( cde->mExtraFieldLength > 0) {
			cde->mExtraField = malloc(cde->mExtraFieldLength + 1);
			memcpy(cde->mExtraField, buf + j + offset, cde->mExtraFieldLength);
			*(cde->mExtraField + cde->mExtraFieldLength) = '\0';
			offset += cde->mExtraFieldLength;
		}
		if ( cde->mFileCommentLength > 0) {
			cde->mFileComment = malloc(cde->mFileCommentLength + 1);
			memcpy(cde->mFileComment, buf + j + offset, cde->mFileCommentLength);
			*(cde->mFileComment + cde->mFileCommentLength) = '\0';
			offset += cde->mFileCommentLength;
		}
		//fprintf(stdout, "read %s\n", cde->mFileName);
		j += offset;
		
		LocalFileHeader* lfh = &((*(mEOCD.mEntries + i)).mLFH);
		unsigned char *tmp = malloc(ZIP_LOCAL_FILE_HEADER);
		lseek(zipFile, cde->mLocalHeaderRelOffset, SEEK_SET);
		reads = read(zipFile, tmp, (size_t)ZIP_LOCAL_FILE_HEADER);
		if (reads == (size_t)ZIP_LOCAL_FILE_HEADER) {
			if (getLongLE(tmp) == ZIP_LFH_SIGNATURE) {
				lfh->mVersionToExtract = getShortLE(tmp + 0x04);
				lfh->mGPBitFlag = getShortLE(tmp + 0x06);
				lfh->mCompressedMethod = getShortLE(tmp + 0x08);
				lfh->mLastModFileTime = getShortLE(tmp + 0x0a);
				lfh->mLastModFileDate = getShortLE(tmp + 0x0c);
				lfh->mCRC32 = getLongLE(tmp + 0x0e);
				lfh->mCompressedSize = getLongLE(&tmp[0x12]);
				lfh->mUnCompressedSize = getLongLE(&tmp[0x16]);
				lfh->mFileNameLength = getShortLE(&tmp[0x1a]);
				lfh->mExtraFieldLength = getShortLE(&tmp[0x1c]);
				if(lfh->mFileNameLength > 0) {
					lfh->mFileName = malloc(lfh->mFileNameLength + 1);
					reads = read(zipFile, lfh->mFileName, lfh->mFileNameLength);
					if( reads > 0) {
						*(lfh->mFileName + lfh->mFileNameLength) = '\0';
					}
				}
				if(lfh->mExtraFieldLength > 0) {
					lfh->mExtraField = malloc(lfh->mExtraFieldLength + 1);
					reads = read(zipFile, lfh->mExtraField, lfh->mExtraFieldLength);
					if( reads > 0) {
						*(lfh->mExtraField + lfh->mExtraFieldLength) = '\0';
					}
				}
				fprintf(stdout, "flag:%d\t%s\n", lfh->mGPBitFlag, lfh->mFileName);
			}
		}
		free(tmp);
	}

	extractZip(zipFile, &mEOCD, outputDir);

	fprintf(stdout, "zipParse Finished\n");

bail:
	destroyEndOfCentralDir(&mEOCD);
	if (NULL != buf) {
		free(buf);
	}
	close(zipFile);
}

int extractZip(int fd, EndOfCentralDir *mEOCD, const char *outputDir) {
	// prepair to extract data.
	struct stat buf;
	if(stat(outputDir, &buf) < 0) {
		fprintf(stderr, "could not read %s state\n", outputDir);
		return -1;
	}
	if(!S_ISDIR(buf.st_mode)) {
		fprintf(stderr, "%s is not a dir\n", outputDir);
		return -1;
	}
	if(access(outputDir, W_OK) < 0) {
		fprintf(stderr, "%s do not has write permission.\n", outputDir);
		return -1;
	}
	fprintf(stdout, "start to extract files\n");
	// "tmp" just use it as target dir name, 1 for '/', 3 for tmp, 1 for '/', and 1 for '0'
	int len = strlen(outputDir);
	printf("%s length:%d\n", outputDir, len);
	char * destDir = malloc(strlen(outputDir) + 1 + 3 + 1 + 1);
	if(NULL == destDir) {
		fprintf(stderr, "memory alloc error\n");
		return -1;
	}
	strcpy(destDir, outputDir);
	*(destDir + len) = '/';
	*(destDir + len + 1) = 't';
	*(destDir + len + 2) = 'm';
	*(destDir + len + 3) = 'p';
	*(destDir + len + 4) = '/';
	*(destDir + len + 5) = '\0';

	if(mkdir(destDir, S_IRUSR|S_IWUSR|S_IXUSR | S_IRGRP | S_IWGRP| S_IROTH)< 0) {
		fprintf(stderr, "can not mkdir %s:%s\n", destDir, strerror(errno));
		goto bad;
	}
	//save current work path, then restore it. I think 100 length is enough.
	char curPath[100];
	getcwd(curPath, 100);
	printf("curPath: %s\n", curPath);
	if(chdir(destDir) < 0) {
		fprintf(stderr, "chdir error:%s\n", strerror(errno));
		goto bad;
	}
	int i;
	for( i = 0 ; i < mEOCD->mTotalNumEntries; i++) {
		ZipEntry *entry = mEOCD->mEntries + i;
		//try to deflate data. from fp to fp.
		inflateToFile(fd, entry);

	}
	if(chdir(curPath) < 0) {
		fprintf(stderr, "chdir error:%s\n", strerror(errno));
		goto bad;
	}
bad:
	if(NULL != destDir) {
		free(destDir);
	}
	return 0;
}

void inflateToFile(int fd, ZipEntry *entry) {
	static const unsigned long kReadBufSize = 32768;

	unsigned char buf[kReadBufSize];

	unsigned long compRemaining = (*entry).mLFH.mCompressedSize;
	//try create file and if it existed, truncate it.
	int destFd = open((char *)(*entry).mLFH.mFileName, O_RDWR, O_CREAT| O_TRUNC);
	if(destFd <= -1) {
		fprintf(stderr, "open destFile error %s\n", strerror(errno));
		return ;
	}
	int zerr;
	z_stream zstream;
	zstream.zalloc = Z_NULL;
	zstream.zfree = Z_NULL;
	zstream.opaque = Z_NULL;
	zstream.next_in = NULL;
	zstream.avail_in = 0;
	zstream.next_out = (Bytef*)buf;
	zstream.avail_out = (*entry).mLFH.mUnCompressedSize;
	zstream.data_type = Z_UNKNOWN;

	zerr = inflateInit2_(&zstream, -MAX_WBITS,ZLIB_VERSION, (int)sizeof(z_stream));
	if (zerr != Z_OK) {
		if (zerr == Z_VERSION_ERROR) {
			fprintf(stderr, "installed zlib is not compatible with linked version (%s)\n", ZLIB_VERSION);
			return ;
		} else {
			fprintf(stderr, "Call to infalteInit2 failed (zerr=%d)\n", zerr);
		}
	}

	long startPos = (*entry).mCDE.mLocalHeaderRelOffset + (ZIP_LOCAL_FILE_HEADER)
	   	+ (*entry).mLFH.mFileNameLength + (*entry).mLFH.mExtraFieldLength;

	do {
		unsigned long getSize;

		if (zstream.avail_in == 0) {
			getSize = (compRemaining > kReadBufSize) ? kReadBufSize : compRemaining;
			fprintf(stdout, "+++ reading %ld bytes (%ld left)\n",
					getSize, compRemaining);
			unsigned char* nextBuffer = malloc(getSize);
			free(nextBuffer);
		}
	} while (zerr == Z_OK);
}

int readEndOfCentralDir(EndOfCentralDir* eocd, unsigned const char * buf, int length) {
	if (length < ZIP_END_OF_CENTEN_DIRECTORY) {
		fprintf(stderr, "readEndOfCentralDir error: invalid position\n");
		return -1;
	}
	
	eocd->mDiskNumber = getShortLE(&buf[0x04]);
	eocd->mDiskWithCentralDir = getShortLE(&buf[0x06]);
	eocd->mNumEntries = getShortLE(&buf[0x08]);
	eocd->mTotalNumEntries = getShortLE(&buf[0x0a]);
	eocd->mCentralDirSize = getLongLE(&buf[0x0c]);
	eocd->mCentralDirOffset = getLongLE(&buf[0x10]);
	eocd->mComment = malloc(getShortLE(&buf[0x14]));
	
	return 0;
}

void destroyLocalFileHeader(LocalFileHeader *lfh) {
	if(NULL != lfh->mFileName) {
		free(lfh->mFileName);
	}
	if(NULL != lfh->mExtraField) {
		free(lfh->mExtraField);
	}
}

void destroyCentralDirEntry(CentralDirEntry *cde) {
	if (NULL != cde->mFileName) {
		free(cde->mFileName);
	}                                                                                                                                                                              

	if (NULL != cde->mExtraField) {
		free(cde->mExtraField);
	}
	
	if (NULL != cde->mFileComment) {
		free(cde->mFileComment);
	}
}

void destroyZipEntry(ZipEntry *entry) {
	destroyCentralDirEntry(&(entry->mCDE));
	destroyLocalFileHeader(&(entry->mLFH));
}

void destroyEndOfCentralDir(EndOfCentralDir *eocd) {
	if ( NULL != eocd->mEntries) {
		int i;
		for( i = eocd->mNumEntries -1 ; i >=0 ; i--) {
			destroyZipEntry(eocd->mEntries + i);
			//destroyCentralDirEntry(eocd->mEntries + i);
		}
		free(eocd->mEntries);
	}

	if (NULL != eocd->mComment) {
		free(eocd->mComment);
	}
}
