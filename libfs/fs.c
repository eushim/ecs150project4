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
	uint8_t padding[4079];
}__attribute__((packed));

struct superblock super;

struct FAT{
	uint16_t * arr;
}__attribute__((packed));

struct FAT ourFAT;

struct entries{
	char filename[16];
	uint32_t file_size;
	uint16_t first_index;
	uint8_t padding[10];
}__attribute__((packed));

struct rootdirectory{
	 struct entries root[128];
}__attribute__((packed));

struct rootdirectory * our_root;

int fs_mount(const char *diskname)
{
	int i;
	void * buffer=NULL;
	if(block_disk_open(diskname)==-1)
		return -1;

	
	//printf("SUPER_SIZE: %lu\n", sizeof(super));
	
	block_read(0,&super);
	//super = buffer;
	char sig[8];
	memcpy(&sig,&super.signature,8);
	if(strcmp("ECS150FS", (char *)sig)!= 0)
		return -1;
	if(super.total_amount!=block_disk_count())
		return -1;
	if(super.num_data!=super.total_amount -2-super.num_FAT)
		return -1;
	if(super.root_index!=super.num_FAT+1)
		return -1;
	if(super.data_index!=super.root_index+1)
		return -1;
	
	
	ourFAT.arr = (uint16_t *)malloc(BLOCK_SIZE*super.num_FAT*sizeof(uint16_t));
	void * temp = (void *) malloc(BLOCK_SIZE);
	for(i=1; i < super.root_index;i++)
	{
		block_read(i,temp);
		memcpy(ourFAT.arr+4096*(i-1),temp,BLOCK_SIZE);
	}
	if(ourFAT.arr[0]!= 0xFFFF)
		return -1;
	buffer = (void *)malloc(BLOCK_SIZE);
	our_root = (struct rootdirectory *)malloc(sizeof(struct rootdirectory));
	block_read(super.root_index,buffer);
	
	//printf("SIZE: %lu\n",sizeof(struct entries));
	for (i=0;i<FS_FILE_MAX_COUNT;i++)
	{
		struct entries * entry = (struct entries*)malloc(sizeof(struct entries));
		memcpy(entry,buffer+(i*32),32); 
		our_root->root[i] = *entry; 
	}
	//printf("SIZE OF ROOT: %lu\n",sizeof(our_root->root));
	return 0;
}

int fs_umount(void)
{
	//printf("RET: %d", ret);
	//free(our_root->root);
	//our_root=NULL;
	//free(our_root);
	//free(ourFAT);

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
	printf("total_blk_count=%i\n",super.total_amount);
	printf("fat_blk_count=%i\n",super.num_FAT);
	printf("rdir_blk=%i\n",super.root_index);
	printf("data_blk=%i\n",super.data_index);
	printf("data_blk_count=%i\n",super.num_data);
	
	for(i=0;i < super.num_data;i++)
	{
		if(ourFAT.arr[i] == 0 )
		{
			freefat++;
		}
	}
	
	printf("fat_free_ratio=%d/%d\n",freefat,super.num_data);
	for(i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries node;
		node = our_root->root[i];
		if (strlen(node.filename) == 0)
			root++;
	}
	
	printf("rdir_free_ratio=%d/%d\n",root ,FS_FILE_MAX_COUNT);
	
	return 0;
	
}

int fs_create(const char *filename)
{
	if (filename == NULL)
		return -1;
		
	int len = strlen(filename);

	if (len+1 > FS_FILENAME_LEN)
		return -1;

	int i, count = 0;	
	
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries node;
		node = our_root->root[i];
		//printf("FILENAME: %s", node.filename);
		if (strlen(node.filename) != 0)
		{
			if (strcmp(node.filename,filename) == 0)
				return -1;
			count += 1;
		}
	}
	
	if (count == FS_FILE_MAX_COUNT)
		{
			return -1;
		}
	
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries node;
		node = our_root->root[i];
		if (strlen(node.filename) == 0)
		{
			strcpy(node.filename, filename);
			printf("INDEX: %i",i);
			printf("FILENAME: %s\n", node.filename);
			node.first_index = 0;
			our_root->root[i] = node;
			block_write(super.root_index,our_root->root);
			break;
		}
	}
	return 0;
}

int fs_delete(const char *filename)
{
	if (filename == NULL)
		return -1;
	
	int nodefound=0;
	int i;
	for (i=0; i< FS_FILE_MAX_COUNT; i++)
	{
		struct entries node;
		node = our_root->root[i];
		if(strcmp(node.filename,filename) == 0)
		{
			nodefound=1;
			ourFAT.arr[node.first_index]=0;
			strcpy(node.filename,"\0");
			our_root->root[i] = node;
			block_write(super.root_index,our_root->root);
			break;
		}
	}
		if(nodefound==0)//for when it cannot find the filename
			return -1;
	return 0;
}

int fs_ls(void)
{
	printf("FS Ls:");
	int i;
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries node;
		node = our_root->root[i];
		if(strlen(node.filename)!=0)
		{
			printf("\nfile: %s, size: %i, data_blk: %i",(char *)node.filename,node.file_size,ourFAT.arr[node.first_index]);
		}
	}
	printf("\n");
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
