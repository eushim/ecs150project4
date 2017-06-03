#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
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

struct fd_node{
	int fd;
	size_t offset;
	const char * filename;
};

struct fd{
	int fd_count;
	struct fd_node fdes[32];
};

struct fd * our_fd;

struct fd_node * retFD(int fd);
struct entries * retEntry(const char * filename);

int fs_mount(const char *diskname)
{
	int i;
	void * buffer=NULL;
	if(block_disk_open(diskname)==-1)
		return -1;

	
	block_read(0,&super);

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
	
	for (i=0;i<FS_FILE_MAX_COUNT;i++)
	{
		struct entries * entry = (struct entries*)malloc(sizeof(struct entries));
		memcpy(entry,buffer+(i*32),32); 
		our_root->root[i] = *entry; 
	}
	
	return 0;
}

int fs_umount(void)
{
	int i;
	void * buffer=malloc(sizeof(BLOCK_SIZE));
	for(i=0;i<super.num_FAT;i++)
	{
		memcpy(buffer,ourFAT.arr+i*4096,BLOCK_SIZE);
		block_write(1+i,buffer);
	}
	block_write(super.root_index,our_root);
	
	int close = block_disk_close();
		
	if (close == -1)
		return -1;
	if(our_fd!=NULL)
	{
		if(our_fd->fd_count!=0)
			return -1;
	}
	return 0;
}

int fs_info(void)
{
	if(block_disk_count()==-1)
		return -1;
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
		if (strlen(node.filename) != 0)
		{
			if (strcmp(node.filename,filename) == 0)
				return -1;
			count += 1;
		}
	}
	
	if (count == FS_FILE_MAX_COUNT)
			return -1;
	
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries node;
		node = our_root->root[i];
		if (strlen(node.filename) == 0)
		{
			strcpy(node.filename, filename);
			node.first_index = 0xFFFF;
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
		
	int found = 0;
	int i;
	
	if (our_fd != NULL)
	{
		for(i=0;i<FS_OPEN_MAX_COUNT;i++)
		{
			struct fd_node node;
			node = our_fd->fdes[i];
			if(strcmp(node.filename,filename)==0)
				found = 1;
		}
		
		if (found == 0)
			return -1;
	}
	
	int nodefound=0;
	for (i=0; i< FS_FILE_MAX_COUNT; i++)
	{
		struct entries entry;
		entry = our_root->root[i];
		if(strcmp(entry.filename,filename) == 0)
		{
			nodefound=1;
			int j = 0;
			int num_blocks = entry.file_size/4096 +1;
			for (j = 0; j < num_blocks; j++) 
			{
				ourFAT.arr[entry.first_index+j] = 0;
				if(	ourFAT.arr[entry.first_index+j]==0xFFFF)
					break;
			}
			

			strcpy(entry.filename,"\0");
			entry.file_size=0;
			entry.first_index=0;
			our_root->root[i] = entry;
			
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
	if(block_disk_count()==-1)
		return -1;
	printf("FS Ls:");
	int i;
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries node;
		node = our_root->root[i];
		if(strlen(node.filename)!=0)
		{
			printf("\nfile: %s, size: %i, data_blk: %i",(char *)node.filename,node.file_size,node.first_index);
		}
	}
	printf("\n");
	return 0;
}

int fs_open(const char *filename)
{
	int found=0;
	if (filename == NULL)
		return -1;
		
	if (our_fd == NULL)
	{
		our_fd = (struct fd *)malloc(sizeof(struct fd));
		int i;
		for (i=0; i<FS_OPEN_MAX_COUNT; i++)
		{
			struct fd_node * n = (struct fd_node*)malloc(sizeof(struct fd_node));
			our_fd->fdes[i] = *n;
		}
		our_fd->fd_count = 0;
	}
	
	if (FS_OPEN_MAX_COUNT == our_fd->fd_count)
		return -1;
	
	int fd=0, i;
	for (i = 0; i < FS_OPEN_MAX_COUNT; i++)
	{
		struct fd_node node;
		node = our_fd->fdes[i];
		if (node.filename == NULL)
		{
			node.filename = filename;
			fd=i;
			node.fd = fd;
			node.offset=0;
			our_fd->fdes[i] = node;
			found=1;
		}
			
	}
	if(found==0)
		return -1;
	our_fd->fd_count +=1; 
	return fd;	
}

int fs_close(int fd)
{
	int i; 
	int found = 0;
	if(fd >= 32 || fd < 0)//check if out of bounds
		return -1;
	
	for (i = 0; i < FS_OPEN_MAX_COUNT; i++)
	{
		struct fd_node node;
		node = our_fd->fdes[i];
		if (node.fd == fd)
		{
			our_fd->fdes[i].filename = NULL;
			found = 1;
		}	
	}
	if (found == 0) //not found
		return -1;
		
	our_fd->fd_count -= 1;
		
	return 0;
}

int fs_stat(int fd)
{
	int found=0;
	int i;
	struct fd_node node;
	if(fd >= 32 || fd < 0)
		return -1;
	for(i=0;i<FS_OPEN_MAX_COUNT;i++)
	{
		node = our_fd->fdes[i];
		if(node.fd==fd)
		{
			found =1;
			break;
		}
	}
	
	if (found == 0)
		return -1;
	
	for(i=0;i<FS_FILE_MAX_COUNT;i++)
	{
		struct entries entry;
		entry = our_root->root[i];
		if(strcmp(entry.filename,node.filename) == 0)
		{
			return entry.file_size;
		}
	}
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	int found = 0;
	int i;
	if(fd >= 32 || fd < 0)
		return -1;
		
	struct fd_node node;
	
	for(i=0;i<FS_OPEN_MAX_COUNT;i++)
	{
		node = our_fd->fdes[i];
		if(node.fd==fd)
		{
			found = 1;
			break;
		}
	}
	
	if (found == 0)
		return -1;
		
	for (i=0; i < FS_FILE_MAX_COUNT; i++)
	{
			struct entries entry;
			entry = our_root->root[i];
			if (strcmp(node.filename, entry.filename) == 0)
			{
				if (offset > entry.file_size)
					return -1;
			}	
	}
	
	node.offset = offset;
	
	return 0;
}

int retFAT(void)
{
	int i;
	for (i = 1; i < super.num_data; i++)
	{
		if (ourFAT.arr[i] == 0)
		{
			return i;
		}
	}
	
	return -1;
}

int countFreeFat(int blocks)
{
	int i, free_count = 0;
	for (i = 1; i < super.num_data; i++)
	{
		if (ourFAT.arr[i] == 0)
		{
			free_count += 1;
			if (free_count == blocks)
				return 1;
		}
	}
	
	return 0;
}

/*int blockIndex(int blockI, int blocks){
	
	int i, free_count = 0;
	for (i = 1; i < blocks; i++)
	{
		if (ourFAT.arr[i] == 0)
		{
			free_count += 1;
			if (free_count == blocks)
				return 1;
		}
	}
	
	
	return -1;
}*/

int fs_write(int fd, void *buf, size_t count)
{
	int i;
	if(fd > 31)
		return -1;
	
	struct fd_node * node = retFD(fd);
	
	if (node == NULL)
		return -1;
		
	struct entries * entry = retEntry(node->filename);
	
	if (entry == NULL)
		return -1;
	
	void *bouncebuffer = (void *) malloc(BLOCK_SIZE);
	
	entry->first_index = retFAT();
	entry->file_size = count;
	int num_blocks = entry->file_size/4096 +1;
	
	int ret_count = countFreeFat(num_blocks);
	
	if (ret_count == 0)
		return -1;
	
	size_t off = count;
	
	for (i = 0; i < num_blocks; i++)
	{
		
		if(off >= BLOCK_SIZE)
			memcpy(bouncebuffer,buf+i*4096,BLOCK_SIZE);
		else
			memcpy(bouncebuffer,buf+i*4096,off);
	
		block_write(super.data_index+entry->first_index+i, bouncebuffer);
		
		if (off >= BLOCK_SIZE)
			node->offset += BLOCK_SIZE;
		else
			node->offset += off;
		
		off -= BLOCK_SIZE;
		
		if (i == num_blocks - 1)
			ourFAT.arr[i+entry->first_index]=0xFFFF;
		else
		 ourFAT.arr[i+entry->first_index]=i+1;	
	}
	
		
	return entry->file_size;
}

struct fd_node * retFD(int fd)
{
	int i;
	struct fd_node * node;
	for(i = 0;i < FS_OPEN_MAX_COUNT; i++)
	{
		node = &our_fd->fdes[i];
		if(node->fd == fd)
			return node;
	}
	return NULL;
}

struct entries * retEntry(const char * filename)
{
	int i;
	for(i = 0;i < FS_FILE_MAX_COUNT; i++)
	{
		struct entries * entry;
		entry = &our_root->root[i];
		if(strcmp(entry->filename,filename) == 0)
			return entry;
	}
	return NULL;
}

int fs_read(int fd, void *buf, size_t count)
{
	if(fd>31)
		return -1;
	struct fd_node * node = retFD(fd);
	void *bouncebuffer = (void *) malloc(BLOCK_SIZE);
	if (node == NULL)
		return -1;
		
	size_t prev_off = node->offset;
	node->offset = 0;
		
	struct entries * entry = retEntry(node->filename);
	
	if (entry == NULL)
		return -1;
	
	if (entry->file_size < count)
		return -1;
	int num_blocks = entry->file_size/4096 +1;


	size_t off = count;
	int i;
	for (i = 0; i < num_blocks; i++)
	{
		block_read(super.data_index+entry->first_index+i,bouncebuffer);
		
		if(off >= BLOCK_SIZE)
			memcpy(buf+i*4096,bouncebuffer,BLOCK_SIZE);
		else
			memcpy(buf+i*4096,bouncebuffer,off);
	
		if (off >= BLOCK_SIZE)
			node->offset += BLOCK_SIZE;
		else
			node->offset += off;
		
		off -= BLOCK_SIZE;
	}
	
	node->offset = prev_off;
		
	return entry->file_size;
}
