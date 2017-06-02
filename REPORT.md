# Intro

# Data Structures

## Superblock
For the superblock struct we put in a signature of 8 bytes that will contain
the value "ECS150FS". We then have a 2 byte value for the number of blocks
that are in the virtual disk. Other 2 bytes values are the root directory
index, data block start index, and the amount of data blocks. We then have 
a one byte value for the number of blocks in the FAT. Finally the remaining
4079 bytes are padding.

## Root Directory
For our root directory, we had an array of 32 byte entries with 128 entries 
total. The 32 byte entries were made with a struct that it composed of a 16 bit
filename, a four byte file size value, a 2 byte first index entry, and 10 byte
padding.

## FAT
The FAT is just an array. We allocate the number of entries in the array in our
`fs_mount()` function based on the total number of data blocks.
## File Descriptor Node
The File Descriptor Node contains the file descriptor number for a currently 
open file. The file descriptor number can be anywhere from 0-31. We also keep 
track of the files current offset and the files name.

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
For this function we iterate through our root directory looking for entries that
are already filled. For the entries that have something in them, we output our 
files names, sizes, and first indices.

## fs_open

## fs_close

## fs_stat
The stat function looks at the file descriptor and finds the file associated 
with the file descriptor. Once the file has been found we go through the root
directory and output the size of the file.
## fs_lseek

## fs_read

## fs_write

# Callback Functions

## RetFAT
This callback function goes through our FAT array and looks for the first
entry that is empty so that we can get the index for first_index when we are
writing a file.

## RetFD
RetFD iterates through our array of open files trying to find the correct file 
descriptor node so that we can get information of the offset and filename.

## RetEntry
This function iterates through our root directory looking for the first empty 
entry so that we can write a files entry information in the root directory.
