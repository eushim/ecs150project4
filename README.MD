# Intro

For this program, we implemented a File System (ECS150FS).

# Data Structures

## Superblock
For the superblock struct we put in a signature of 8 bytes that will contain
the value "ECS150FS". We then have a 2 byte value for the number of blocks
that are in the virtual disk. Other 2 bytes values are the root directory
index, data block start index, and the amount of data blocks. We then have 
a one byte value for the number of blocks in the FAT. Finally the remaining
4079 bytes are padding.

    struct superblock{
    	uint64_t signature;
    	uint16_t total_amount;
    	uint16_t root_index;
    	uint16_t data_index;
    	uint16_t num_data;
    	uint8_t num_FAT;
    	uint8_t padding[4079];
    }__attribute__((packed));

## Root Directory
For our root directory, we had an array of 32 byte entries with 128 entries 
total. The 32 byte entries were made with a struct that it composed of a 16 bit
filename, a four byte file size value, a 2 byte first index entry, and 10 byte
padding.

    struct entries{
    	char filename[16];
    	uint32_t file_size;
    	uint16_t first_index;
    	uint8_t padding[10];
    }__attribute__((packed));

    struct rootdirectory{
	    struct entries root[128];
    }__attribute__((packed));

## FAT
The FAT is just an array. We allocate the number of entries in the array in our
`fs_mount()` function based on the total number of data blocks.

    struct FAT{
	    uint16_t * arr;
    }__attribute__((packed));


## File Descriptor Node
The File Descriptor Node contains the file descriptor number for a currently 
open file. The file descriptor number can be anywhere from 0-31. We also keep 
track of the files current offset and the files name.

    struct fd_node{
    	int fd;
    	size_t offset;
    	const char * filename;
    };

    struct fd{
	    int fd_count;
	    struct fd_node fdes[32];
    };

# Functionality

## fs_mount
In `fs_mount`, we first read in block 0 of our disk to get the values we need
for our superblock struct. We then go through and read the blocks that contain
the values we have for our FAT. We then put what we got from those blocks into
the FAT array. The last thing we do is read the block containing our root 
directory. We divide the buffer that read that block into the 128 entries and 
put them in our entries array in our root directory struct.

## fs_unmount
In our `fs_unmount` function, we write to our FAT blocks and our root directory 
block so that the data is stored onto the disk before we close our disk.

## fs_info
In the `fs_info` function, we display the total block count that is on our 
virtual disk. We also report the number of fat blocks, our root directory index,
initial data block index, data block count, fat free ratio and root directory
ratio. We get most of these values from our structs except for the fat free 
ratio and root directory free ratio. We figured out these ratios by iterating
through our structs to see if the values were zero.

## fs_create
For our `fs_create` function we first check to see if the file name is valid
and that the filename does not already exist in our root directory. We then 
put the filename in the first available spot of the root directory and set the
initial size to be zero and set the first_index to `FAT_EOC`.

## fs_delete
In the 'fs_delete' fucntion we check that the file name is valid and that the
current file is not open. We then iterate through our root_directory changing
the file name, size and first index back to zero. We also go through our FAT
and set the values to zero for parts of the array that were filled by the file.

## fs_ls
For this function we iterate through our root directory looking for entries 
that are already filled. For the entries that have something in them, we output 
our files names, sizes, and first indices.

## fs_open
For this function we initialize our fd struct which contains the fd count and 
fd array that has all the file descriptors. Then we check to see if the max 
count of files open is reached and if it hasn't been reached we then proceed to 
add the new file being opened. The file that is being added is then added to 
the file descriptor array fdes and the fd_count is incremented.

## fs_close
For this function we check to see if the file being closed is found in the fdes
array and if it is not it will return -1. If the file is found then we simply 
remove it from the fdes by making the filename equal to NULL and by 
decrementing the fd_count.

## fs_stat
The stat function looks at the file descriptor and finds the file associated 
with the file descriptor. Once the file has been found we go through the root
directory and output the size of the file.
## fs_lseek
The lseek function looks at the fd and then finds the file associated with the 
file descriptor. Once the file has been found it will then set the offset that 
is being passed though the function.

## fs_read
The read function looks at the fd and then finds the file associated with the 
file descriptor. Once the file has been found it will then use the block_read
function at super data_index where the file can be found. This is being done 
with a for loop and uses memcpy to add what has been read from the block to the
buf. Also the offset is being set continously during the for loop and then
becomes reset at the end of the function.

## fs_write
The write function looks at the fd and then finds the file associated with the 
file descriptor. Once the file has been found it will then use a buffer to 
memcpy from the given buf into a block. Every iteration then becomes written to
a block at the super data_index where the file is found. Also the offset is 
being set continously during the for loop and is stored once the function has
finished.

# Callback Functions

## retFAT()
This callback function goes through our FAT array and looks for the first
entry that is empty so that we can get the index for first_index when we are
writing a file.

    int retFAT(void)
    {
    	int i;
    	for (i = 1; i < super.num_data; i++)
    	{
    		if (ourFAT.arr[i] == 0)
    			return i;
    	}
    	return -1;
    }


## RetFD()
RetFD iterates through our array of open files trying to find the correct file 
descriptor node so that we can get information of the offset and filename.

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

## RetEntry()
This function iterates through our root directory looking for the first empty 
entry so that we can write a files entry information in the root directory.
    
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
