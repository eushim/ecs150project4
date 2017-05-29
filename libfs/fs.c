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
}__attribute__((__packed__));

struct superblock * super;

struct FAT{
	uint16_t * arr;
}__attribute__((__packed__));

struct FAT * ourFAT;

struct entries{
	char filename[16];
	uint32_t file_size;
	uint16_t first_index;
	uint8_t padding[10];
}__attribute__((__packed__));

struct entries * entry;
struct rootdirectory{
	struct entries *root[128];
}__attribute__((__packed__));

struct rootdirectory * our_root;

int fs_mount(const char *diskname)
{
	int i;
	void * buffer=(void *)malloc(BLOCK_SIZE);
	if(block_disk_open(diskname)==-1)
		return -1;
	super = (struct superblock *)malloc(sizeof(struct superblock));

	block_read(0,buffer);
	super =buffer;
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
	//makbuffer=(void *)malloc(BLOCK_SIZE);
		//entry = (struct entries*)malloc(sizeof(struct entries));
	our_root = (struct rootdirectory *)malloc(sizeof(struct rootdirectory));
	block_read(super->root_index,buffer);
	//memcpy(our_root->root,buffer,4096);
	printf("%i",sizeof(struct entries));
	for (i=0;i<FS_FILE_MAX_COUNT;i++)
	{
		entry = (struct entries*)malloc(sizeof(struct entries));
		memcpy(entry,buffer+(i*32),32);
		our_root->root[i] = entry;
	}
	
	//entry = our_root->root[0];
	//strcpy(entry->filename,"hello");
	
	return 0;
}

int fs_umount(void)
{
	//block_write(0,super);
	//block_write(1,ourFAT);
	block_write(super->root_index,our_root);
	//free(our_root->root);
	//our_root=NULL;
	free(our_root);
	free(ourFAT);
	free(super);

	int close = block_disk_close();
		
	if (close == -1)
		return -1;

	return 0;
}

int fs_info(void)
{
	if (our_root==NULL)
		return -1;
	
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
		//printf("filename: %s", (char*)node->filename);
		if (node->filename != NULL)
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
	printf("STR: %c\n", filename[len]);
	if (filename[len] != '\0' || len+1 > FS_FILENAME_LEN)
		return -1;

	int i, count = 0;
	int index = 0;
	struct entries * node = NULL;
	
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		node = our_root->root[i];
		char * name = (char *)node->filename;
		printf("filename: %s", name);
		if (name != NULL)
		{
			if (strcmp(name,filename) == 0)
				return -1;
		}
		if (node->filename != 0)
			count += 1;
	}
	
	if (count == FS_FILE_MAX_COUNT)
		return -1;
	
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		node= our_root->root[i];
		char * name = (char *)node->filename;
		printf("filename: %s\n", name);
		if (node->filename == 0)
		{
			strcpy(node->filename, filename);
			name = (char *)node->filename;
			printf("%s\n",(char *) node->filename);
			index=i;
			printf("INDEX: %d\n", index);
			our_root->root[i] = node;
			//if(block_write(super->root_index,our_root)==-1)
			//	return -1;
			printf("filename: %s\n", name);
			entry=our_root->root[i];
			break;
		}
			
	}
	//entry->filename = (uint128_t*)filename;
	//printf("%s\n",(char *) entry->filename);
	//entry->file_size = 0;
	//node->first_index = count+1;
	//printf("Index: %i\n",index);
	//our_root->root[index] = entry;
	
	
	//memcpy(our_root->root[index],node,32);
	//printf("%i\n",node->first_index);
	//block_write(super->root_index,our_root);
	//ourFAT->arr[node->first_index]=0XFFFF;
//	printf("%i\n",ourFAT->arr[node->first_index]);
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
		struct entries * node;
		node = our_root->root[i];
		if((char *)node->filename==filename)
		{
			nodefound=1;
			ourFAT->arr[node->first_index]=0;
			our_root->root[i]=NULL;
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
			struct entries * node;
		node = our_root->root[i];
		if(node->filename!=NULL)
		{
			printf("\nfile: %s,size: %i, data_blk: %i",(char *)node->filename,node->file_size,ourFAT->arr[node->first_index]);
		}
	}
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

