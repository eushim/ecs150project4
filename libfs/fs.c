#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "disk.h"
#include "fs.h"

struct __attribute__((__packed__)) superblock{
	uint64_t signature;
	uint16_t total_amount;
	uint16_t root_index;
	uint16_t data_index;
	uint16_t num_data;
	uint8_t num_FAT;
};




struct __attribute__((__packed__)) rootdirectory{
	void * filename;
	uint32_t file_size;
	uint16_t first_index;
};

struct __attribute__((__packed__)) FAT{
	void * arr;
};


int fs_mount(const char *diskname)
{
	void * superbuf=(void *)malloc(BLOCK_SIZE);
	if(block_disk_open(diskname)==-1)
		return -1;
	struct superblock * super= (struct superblock *)malloc(sizeof(struct superblock));
	super->signature = (uint64_t) "ECS150FS";
	super->num_data = block_disk_count();
	if(super->num_data==-1)
		return -1;
	super->num_FAT=(super->total_amount*2)/4096;
	if((super->total_amount*2)%4096!=0)
		super->num_FAT++;//round up if extra blocks
	super->root_index = 1 + super->num_FAT;
	super->data_index= super->root_index+1;
	super->total_amount= super->num_data+2+super->num_FAT;
	memcpy(superbuf,super,BLOCK_SIZE);
	block_write(0,superbuf);
	return 0;
}

int fs_umount(void)
{
	block_disk_close();
	
	return 0;
}

int fs_info(void)
{
	return 0;
}

int fs_create(const char *filename)
{
	if (filename == NULL)
		return -1;
		
	return 0;
}

int fs_delete(const char *filename)
{
	if (filename == NULL)
		return -1;
	
	return 0;
}

int fs_ls(void)
{
	return 0;
}

int fs_open(const char *filename)
{
	if (filename == NULL)
		return -1;
		
	return 0;
}

int fs_close(int fd)
{
	return 0;
}

int fs_stat(int fd)
{
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	return 0;
}

