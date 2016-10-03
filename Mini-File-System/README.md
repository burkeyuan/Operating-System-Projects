(1) Data structure of the mini file system   
      The file system or disk file is consisted of 64 blocks, each of which has 16 bytes. 32 blocks are for metadata and the other 32 blocks are for file data. The type of all data stored in the file system is unsigned char and each character’s size is one byte. So we can store 16 characters in a block. The metadata of the file system including the root directory, file allocation table and open file table. In my design, the root directory occupies 4 blocks(64 bytes) in the disk file and it is also stored in array DIR[8][8] in memory for operation. It can store the information of eight files. Let use i to denote the index of a file. DIR[i][0] denotes the status of a file. DIR[i][1] denotes the number of the first block containing the file data. DIR[i][2] to DIR[i][5] denote the file name with at most four characters. DIR[i][6] denotes the length of the file. At last, DIR[i][7] denotes the corresponding file descriptor if the file is open. The file allocation table occupies 4 blocks(64 bytes) in the disk file and it is also stored in array FAT[32][2] in memory. Let’s use i to denote the index of a data block. FAT[i][0] denotes the status of the block, free or used. FAT[i][1] denotes the number of the next block containing the a file’s data if the file exists. After required operations, the root directory and file allocation table in the disk file will be updated according to DIR[8][8] and FAT[32][2]. The open file table is only stored in memory and will be reset to original state after is disk is dismounted and closed. OFT[i][0] denotes the status of the descriptor i. OFT[i][1] denotes the index of corresponding file. OFT[i][0] denotes the offset in the file. Each data block can only be allocated to a file at a time whether it is full. A data block cannot be used by two files at a time.
(2) File system functions
      Based on the functions in disk.c, I design the 12 file system functions.
(2.1) make_fs(char *disk_name)
      Firstly, check whether the disk_name is legal. If the disk_name is consisted of no more than 4 characters, it is legal and I use make_disk() function to make a file system and create a disk file. Then I initialize the three tables in memory: DIR[8][8], FAT[32][2] and OFT[4][3]. 
(2.2) mount_fs(char *disk_name)
      Firstly, open the disk file and copy the metadata in the disk file to buffers in memory by using block_read() function. The metadata is read block by block. Then update the information of DIR[8][8] and FAT[32][2] according to the data structure of the metadata.
(2.3) dismount_fs(char *disk_name)
      Firstly, copy 16 bytes of the DIR[8][8] or FAT[32][2] every time to a buffer. And then use block_write() function copy the data in the buffer to the disk file. OFT[4][3] do not need to be copied. At last, close the disk file with close_disk() function and reset DIR[8][8], FAT[32][2] and OFT[4][3] to the original state.
(2.4) fs_open(char *name)
      Firstly, check whether the file exists, whether the file is already open and whether there is available file descriptor for use. If not, there is a failure in opening the file and return -1. Otherwise, search for an available file descriptor with first fit algorithm. If the file is opened for the first time, search for an available data block with first fit algorithm. Then update the relevant information and return the file descriptor.
(2.5) fs_close(char *name)
      Just update relevant information in DIR[8][8] and OFT[4][3]. And decrease the number of used file descriptors.
(2.6) fs_create(char *name)
      Firstly, check whether the file with name exists or not and whether the number of files exceeds 8. Then search for an available index in DIR[8][8] with first fit algorithm.
(2.7) fs_delete(char *name)
      Firstly, check whether the file with name exists and whether the file is open or not. Then free the data blocks of the file by set the status of these blocks to 0. At last, remove all the information of the file in DIR[8][8].
(2.8) fs_read(int fildes, void *buf, size_t nbyte)
      Firstly, copy all the data of the file in the disk file to a buffer in memory. Then copy the required part of the file to buf[] according to the offset and nbyte. 
(2.9) fs_write(int fildes, void *buf, size_t nbyte)
      Consider different situations and handle them one by one. When the file pointer is at the beginning, the end or between the beginning and the end of the file, different solutions are provided.
(2.10) fs_get_filesize(int fildes)
      Just return the length of file stored in DIR[8][8].
(2.11) fs_lseek(int fildes, off_t offset)
      Update the offset of an open file in OFT[4][3].
(2.12) fs_truncate(int fildes, off_t offset)
       Calculate the blocks the file needs after truncating. And free the blocks the file do not need any more. Then update the information of DIR[8][8] and FAT[32][2].

