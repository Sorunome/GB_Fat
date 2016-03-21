#ifndef _gb_fat_h_
#define _gb_fat_h_

#define SPISPEED_LOW				0x03
#define SPISPEED_MEDIUM				0x02
#define SPISPEED_HIGH				0x01
#define SPISPEED_VERYHIGH			0x00


#define NO_ERROR					0x00
#define ERROR_NO_MORE_FILES			0x01
#define ERROR_FILE_NOT_FOUND		0x10
#define ERROR_ANOTHER_FILE_OPEN		0x11
#define	ERROR_MBR_READ_ERROR		0xF0
#define	ERROR_MBR_SIGNATURE			0xF1
#define	ERROR_MBR_INVALID_FS		0xF2
#define ERROR_BOOTSEC_READ_ERROR	0xE0
#define	ERROR_BOOTSEC_SIGNATURE		0xE1
#define ERROR_NO_FILE_OPEN			0xFFF0
#define ERROR_WRONG_FILEMODE		0xFFF1
#define FILE_IS_EMPTY				0xFFFD
#define BUFFER_OVERFLOW				0xFFFE
#define EOF							0xFFFF


#include <inttypes.h>
struct _boot_sector{
	uint8_t sectorsPerCluster;
	uint16_t fat1Start;
	uint32_t ClusterOffset;
};


class GB_File
{
public:
	GB_File(uint16_t cluster = 0);
	void read(uint8_t *buffer,uint32_t start,uint16_t size);
	uint8_t exists();
private:
	uint16_t startCluster;
};

class GB_Fat
{
private:
	uint16_t start_cluster;
	uint8_t isNextFile(char *fn,uint8_t *buffer);
	unsigned long firstDirSector;
	uint16_t offset;
	uint16_t currSec;
public:
	uint8_t init(uint8_t *buffer,uint8_t speed=SPISPEED_HIGH);
	GB_File open(char *fn,uint8_t *buffer);
};


#endif