#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "disk.h"
#include "fs.h"

typedef unsigned __int128 uint128_t;

struct superblock{
	uint64_t signature;
	uint16_t total_amount;
	uint16_t root_index;
	uint16_t data_index;
	uint16_t num_data;
	uint8_t num_FAT;
}__attribute__((__packed__));

struct superblock * super;

struct FAT{
	uint16_t * arr;
}__attribute__((__packed__));

struct FAT * ourFAT;

struct entries{
	uint128_t filename;
	uint32_t file_size;
	uint16_t first_index;
}__attribute__((__packed__));

struct entries * entry;

struct rootdirectory{
	struct entries ** root;
};

struct rootdirectory * our_root;

int fs_mount(const char *diskname)
{
	int i;
	void * buffer=(void *)malloc(BLOCK_SIZE);
	if(block_disk_open(diskname)==-1)
		return -1;
	super = (struct superblock *)malloc(sizeof(struct superblock));
	block_read(0, buffer);

	super = buffer;
	
	char sig[8];
	memcpy(&sig,&super->signature,8);
	if(strcmp("ECS150FS", (char *)sig)!= 0)
		return -1;
	if(super->total_amount!=block_disk_count())
		return -1;
	if(super->num_data!=super->total_amount -2-super->num_FAT)
		return -1;
	if(super->root_index!=super->num_FAT+1)
		return -1;
	if(super->data_index!=super->root_index+1)
		return -1;
		
	ourFAT = (struct FAT*)malloc(sizeof(struct FAT));
	ourFAT->arr = (void *)malloc(BLOCK_SIZE*super->num_FAT);
	for(i=1; i < super->root_index;i++)
	{
		block_read(i,buffer);
		memcpy(ourFAT->arr+BLOCK_SIZE*(i-1),buffer,BLOCK_SIZE);
	}
	if(ourFAT->arr[0]!= 0xFFFF)
		return -1;
	
	entry = (struct entries *)malloc(sizeof(struct entries));
	
	our_root = (struct rootdirectory *) malloc(sizeof(struct rootdirectory));
	our_root->root = malloc(FS_FILE_MAX_COUNT);
	block_read(super->root_index,buffer);
	for (i=0;i<FS_FILE_MAX_COUNT;i++)
	{
		memcpy(entry,buffer+i*32,32);
		our_root->root[i] = entry;
	}
	
	return 0;
}

int fs_umount(void)
{
/*	fs_umount - Unmount file system
 *
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 *
 * Return: -1 if no underlying virtual disk was opened, or if the virtual disk
 * cannot be closed, or if there are still open file descriptors. 0 otherwise.*/
	
	//free(our_root->root[0]);
	//free(entry);
	free(ourFAT);
	free(super);
	
	
	
	 //root_entry_t entry = NULL;
	
	//*entry = our_root->root[1];
	//free(entry);
	
	int close = block_disk_close();
		
	if (close == -1)
		return -1;
	return 0;
}

int fs_info(void)
{
	int freefat = 0, i;
	int root = 0;
	printf("FS Info:\n");
	printf("total_blk_count=%i\n",super->total_amount);
	printf("fat_blk_count=%i\n",super->num_FAT);
	printf("rdir_blk=%i\n",super->root_index);
	printf("data_blk=%i\n",super->data_index);
	printf("data_blk_count=%i\n",super->num_data);
	
	for(i=0;i < super->num_data;i++)
	{
		if(ourFAT->arr[i] == 0 )
		{
			freefat++;
		}
	}
	
	printf("fat_free_ratio=%d/%d\n",freefat,super->num_data);
	for(i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries * node;
		node = our_root->root[i];
		if (node->filename == 0)
			root++;
	}
	
	printf("rdir_free_ratio=%d/%d\n",root ,FS_FILE_MAX_COUNT);
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

