#include "GB_Fat.h"
#include "mmc.h"
#include <inttypes.h>

_boot_sector GB_BS;
uint8_t GB_Fat::init(uint8_t *buffer,uint8_t speed){
	unsigned long MBR_part1Start;
	
	mmc::initialize(speed);
	if(RES_OK == mmc::readSector(buffer,0)){
		if((buffer[0x01FE]==0x55) && (buffer[0x01FF]==0xAA)){
			MBR_part1Start = (uint16_t(buffer[454])+(uint16_t(buffer[455])<<8)+(uint32_t(buffer[456])<<16)+(uint32_t(buffer[457])<<24));
		}else{
			return ERROR_MBR_SIGNATURE;
		}
	}else{
		return ERROR_MBR_READ_ERROR;
	}
	if((buffer[450]!=0x04) && (buffer[450]!=0x06) && (buffer[450]!=0x86)){
		return ERROR_MBR_INVALID_FS;
	}
	
	if (RES_OK == mmc::readSector(buffer, MBR_part1Start)){
		if((buffer[0x01FE]==0x55) && (buffer[0x01FF]==0xAA)){
			
			GB_BS.fat1Start = MBR_part1Start + uint16_t(buffer[0x0E])+(uint16_t(buffer[0x0F])<<8); // firstDirSector
			
			firstDirSector = GB_BS.fat1Start;
			
			firstDirSector += uint32_t(buffer[0x10]) * // fat copies
				(uint32_t(buffer[0x16])+(uint32_t(buffer[0x17])<<8)); // sectors per fat
			
			
			GB_BS.ClusterOffset = firstDirSector+(((uint32_t(buffer[0x11])+(uint32_t(buffer[0x12])<<8))*32) / // root directory entries
				512); // bytes per sector
			
			GB_BS.sectorsPerCluster = buffer[0x0D];
		}else{
			return ERROR_BOOTSEC_SIGNATURE;
		}
	}else{
		return ERROR_BOOTSEC_READ_ERROR;
	}
	
	return NO_ERROR;
}

uint8_t GB_Fat::isNextFile(char *fn,uint8_t *buffer){
	uint8_t fn_i = 0,fn_j = 0;
	char tmpFn[13];
	
	while (offset>=512){
			currSec++;
			offset-=512;
	}
	
	mmc::readSector(buffer, currSec);

	if(buffer[0]==0x00){
		return ERROR_NO_MORE_FILES;
	}
	
	while((buffer[offset + 0x0B] & 0x08) || (buffer[offset + 0x0B] & 0x10) || (buffer[offset]==0xE5)){
		offset += 32;
		if(offset==512){
			currSec++;
			mmc::readSector(buffer, currSec);
			offset = 0;
		}
		if (buffer[offset]==0x00){
			return ERROR_NO_MORE_FILES;
		}
	}
	
	
	while((buffer[fn_i + offset]!=0x20) && (fn_i < 8)){
		tmpFn[fn_i] = buffer[fn_i + offset];
		fn_i++;
	}
	tmpFn[fn_i] = '.';
	fn_i++;
	while((buffer[fn_j + offset + 0x08]!=0x20) && (fn_j < 3)){
		tmpFn[fn_i] = buffer[fn_j + offset + 0x08];
		fn_i++;
		fn_j++;
	}
	tmpFn[fn_i] = 0x00;
	
	if(strcmp(tmpFn,fn)!=0){
		offset += 32;
		return ERROR_FILE_NOT_FOUND;
	}
	
	return NO_ERROR;
}

GB_File GB_Fat::open(char *fn,uint8_t *buffer){
	uint8_t res;
	offset=0;
	currSec = firstDirSector;
	res = isNextFile(fn,buffer);
	while(res==ERROR_FILE_NOT_FOUND){
		res = isNextFile(fn,buffer);
	}
	
	if(res == NO_ERROR){
		
		return GB_File(uint16_t(buffer[0x1A + offset]) + (uint16_t(buffer[0x1B + offset])<<8));
	}
	return GB_File(0);
}

GB_File::GB_File(uint16_t cluster){
	startCluster = cluster;
}
void GB_File::read(uint8_t *buffer,uint32_t start,uint16_t size){
	uint16_t readCluster = startCluster;
	uint8_t sectorOffset = -1;
	uint16_t real_size;
	uint8_t buf[2];
	while(start >= 512){
		start -= 512;
		sectorOffset++;
		if(sectorOffset >= GB_BS.sectorsPerCluster){
			sectorOffset = 0;
			mmc::readSector(buf,GB_BS.fat1Start + ((readCluster >> 8) & 0xFF),(readCluster % 256)*2,2);
			readCluster = buf[0] + (buf[1]<<8);
			if(readCluster == 0xffff){
				return;
			}
		}
	}
	do{
		sectorOffset++;
		if(sectorOffset >= GB_BS.sectorsPerCluster){
			sectorOffset = 0;
			mmc::readSector(buf,GB_BS.fat1Start + ((readCluster >> 8) & 0xFF),(readCluster % 256)*2,2);
			readCluster = buf[0] + (buf[1]<<8);
			if(readCluster == 0xffff){
				return;
			}
		}
		
		uint16_t sec = GB_BS.ClusterOffset +
				((uint32_t)GB_BS.sectorsPerCluster * (uint32_t)(readCluster - 2)) + sectorOffset;
		real_size = 512 - (start % 512);
		if(size < real_size){
			real_size = size;
		}
		mmc::readSector(buffer, sec, start % 512, real_size);
		buffer += real_size;
		size -= real_size;
		start = 0; // every following will be alligned correctly anyways
	}while(size);
}
uint8_t GB_File::exists(){
	return startCluster != 0;
}