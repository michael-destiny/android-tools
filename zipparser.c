#include "apkparse.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void openZip(const char *zipFileName) {
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
	
	mEOCD.mEntries = malloc(mEOCD.mTotalNumEntries * sizeof(CentralDirEntry));
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
		CentralDirEntry *entry = mEOCD.mEntries + i;
		if(getLongLE(&buf[j]) != ZIP_CDE_SIGNATURE) {
			fprintf(stderr, "read CentralDirEntry signature error.\n");
			goto bail;
		}
		entry->mVersionMadeBy = getShortLE(buf + j + 0x04);
		entry->mVersionToExtract = getShortLE(buf + j + 0x06);
		entry->mGPBitFlag = getShortLE(buf + j + 0x08);
		entry->mCompressionMethod = getShortLE(buf + j + 0x0a);
		entry->mLastModFileTime = getShortLE(buf + j + 0x0c);
		entry->mLastModFileDate = getShortLE(buf + j + 0x0e);
		entry->mCRC32 = getLongLE(buf + j + 0x10);
		entry->mCompressedSize = getLongLE(buf + j + 0x14);
		entry->mUnCompressedSize = getLongLE(buf + j + 0x18);
		entry->mFileNameLength = getShortLE(buf + j + 0x1c);
		entry->mExtraFieldLength = getShortLE(buf + j + 0x1e);
		entry->mFileCommentLength = getShortLE(buf + j + 0x20);
		entry->mDiskNumberStart = getShortLE(buf + j + 0x22);
		entry->mInternalAttrs = getShortLE(buf + j + 0x24);
		entry->mExternalAttrs = getShortLE(buf + j + 0x26);
		entry->mLocalHeaderRelOffset = getLongLE(buf + j + 0x2a);
		int offset = 0x2e;
		if ( entry->mFileNameLength > 0) {
			entry->mFileName = malloc(entry->mFileNameLength + 1);
			memcpy(entry->mFileName, buf + j + offset, entry->mFileNameLength);
			*(entry->mFileName + entry->mFileNameLength)= '\0';
			offset += entry->mFileNameLength;
		}
		if ( entry->mExtraFieldLength > 0) {
			entry->mExtraField = malloc(entry->mExtraFieldLength + 1);
			memcpy(entry->mExtraField, buf + j + offset, entry->mExtraFieldLength);
			*(entry->mExtraField + entry->mExtraFieldLength) = '\0';
			offset += entry->mExtraFieldLength;
		}
		if ( entry->mFileCommentLength > 0) {
			entry->mFileComment = malloc(entry->mFileCommentLength + 1);
			memcpy(entry->mFileComment, buf + j + offset, entry->mFileCommentLength);
			*(entry->mFileComment + entry->mFileCommentLength) = '\0';
			offset += entry->mFileCommentLength;
		}
		fprintf(stdout, "read %s\n", entry->mFileName);
		j += offset;
	}
	fprintf(stdout, "zipParse Finished\n");

bail:
	destroyEndOfCentralDir(&mEOCD);
	if (NULL != buf) {
		free(buf);
	}
	close(zipFile);
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

void destroyEndOfCentralDir(EndOfCentralDir *eocd) {
	if ( NULL != eocd->mEntries) {
		int i;
		for( i = eocd->mNumEntries -1 ; i >=0 ; i--) {
			destroyCentralDirEntry(eocd->mEntries + i);
		}
		free(eocd->mEntries);
	}

	if (NULL != eocd->mComment) {
		free(eocd->mComment);
	}
}
