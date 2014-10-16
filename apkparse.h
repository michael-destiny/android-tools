#ifndef __APKPARSER_H
#define __APKPARSER_H
#include <unistd.h>
#include <stdlib.h>

#define ZIP_END_OF_CENTEN_DIRECTORY		22
// if you use #define, it would just regard it as int.
//#define ZIP_END_OF_CENTEN_DIRECTORY_SEARCH	(long)ZIP_END_OF_CENTEN_DIRECTORY + 65535

static long ZIP_END_OF_CENTEN_DIRECTORY_SEARCH = ZIP_END_OF_CENTEN_DIRECTORY + 65535;

#define ZIP_END_OF_CENTEN_DIRECTORY_SIGN				0x06054b50

typedef struct {
	char		*fileName;
	char		*outputFile;
	char		*inputFile;
} apkFile;

typedef struct {
	unsigned short mVersionMadeBy;
	unsigned short mVersionToExtract;
	unsigned short mGPBitFlag;
	unsigned short mCompressionMethod;
	unsigned short mLastModFileTime;
	unsigned short mLastModFileDate;
	unsigned long  mCRC32;
	unsigned long  mCompressedSize;
	unsigned long  mUnCompressedSize;
	unsigned short mFileNameLength;
	unsigned short mExtraFieldLength;
	unsigned short mFileCommentLength;
	unsigned short mDiskNumberStart;
	unsigned short mInternalAttrs;
	unsigned long  mExternalAttrs;
	unsigned long  mLocalHeaderRelOffset;
	unsigned char* mFileName;
	unsigned char* mExtraField;
	unsigned char* mFileComment;

	enum {
		ZIP_CDE_SIGNATURE =					0x02014b50
	};

} CentralDirEntry;

typedef struct {
	unsigned short mDiskNumber;
	unsigned short mDiskWithCentralDir;
	unsigned short mNumEntries;
	unsigned short mTotalNumEntries;
	unsigned long	mCentralDirSize;
	unsigned long	mCentralDirOffset;
	unsigned short	mCommentLen;
	unsigned char * mComment;
	CentralDirEntry* mEntries;

	enum {
		ZIP_EOCD_SIGNATURE			=		0x06054b50
	};
} EndOfCentralDir;

void 
openZip(const char* zipFileName);

int
readEndOfCentralDir(EndOfCentralDir* eocd, unsigned const char * buf, int length);

int 
readCentralDirEntry(CentralDirEntry * cde, unsigned const char * buf, int length);

void destroyCentralDirEntry(CentralDirEntry * cde);

void destroyEndOfCentralDir(EndOfCentralDir* eocd);

/*
 *	"LE" means little endian.
 */
static inline unsigned short getShortLE(unsigned const char *buf) {
	return buf[0] | (buf[1] << 8);
}

static inline unsigned long getLongLE(unsigned const char *buf) {
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static inline off_t tell(int fd) {
	return lseek(fd, 0, SEEK_CUR);
}
#endif
