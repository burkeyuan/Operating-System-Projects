#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "disk.h"

#define BUFFER_SIZE 80
#define BLOCK_SIZE 16
#define DISK_BLOCKS 64

/*parameters which are also used in disk.c or myApp.c*/
static int active = 0;  
static int handle;      

size_t nbyte;
off_t offset;
off_t length;

int fda, fdb, fdc, fdd, fde;

/*parameters recording the numbers and indexes*/
int num_file = 0;
int num_fildes = 0;
int index_file = 0;
int index_FAT = 0;
int index_block = 0;
int index_filedes = 0;

/*Arrays used to store the metadata
  DIR[8][8]: the information of at most eight files(Directory)
             0-status 1-first block number 2-5-file name which has most four characters
             6-the length of file 7-file descriptor
  FAT[32][2]: the information of 32 data blocks(File Allocation Table)
              0-status 1-block number
  OFT[4][3]: the information of open files(Open File Table)
             0-status 1-corresponding file's index in DIR[][] 2-offset*/ 
char DIR[8][8];
char FAT[32][2];
char OFT[4][3];

/*make_fs() function: 
  make the file system and initialize the arrays in memory which stores metadata*/
int make_fs(char *disk_name) {
  char *p = disk_name;   
  int flag = 0; 
  int count = 0;
  int i = 0, j = 0;

//check if the file name is legal: no more than 4 characters and consists of only characters 
  while (*p != '\0') {
    count++;
    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
        p++;    
    }else{
        flag = 1;
        break;
    }        
  }
  
  if (flag == 1 || count > 4) {
    printf("Illeagal file name\n");
    return -1;
  }

/*make the disk file by make_disk() function and initialize the metadata(DIR, FAT and OFT) 
  DIR[i][7] and OFT[i][1] are set to -1 at first*/   
  if (make_disk(disk_name) == 0) {
    for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++) {
        if(j == 7){
          DIR[i][j] = -1;
        }
        DIR[i][j] = 0;
      }
    }

    for (i = 0; i < 32; i++) {
      for (j = 0; j < 2; j++) {
        FAT[i][j] = 0;
      }
    }

    for (i = 0; i < 4; i++) {
      OFT[i][0] = 0;
      OFT[i][1] = -1;
      OFT[i][2] = 0; 
    }
  }else{
    return -1;
  }

  return 0;
}

/*mount_fs() function:
 open the disk and copy the metadata from disk file to buffers in memory,  
 then copy the data to DIR[][] and FAT[][] according to the data structure
 buffer1[64]: the information of DIR
 buffer2[64]: the information of FAT
 buffer[16]: used as a medium buffer because block_read() can only 16 characters at a time*/
int mount_fs(char *disk_name) {
  int i = 0, j = 0, k = 0;
  unsigned char buffer1[64];
  unsigned char buffer2[64];
  unsigned char buffer[16];

  open_disk(disk_name);

  for (i = 0; i < 4; i++) {
    block_read(i + 1, buffer);
    for (j = 0; j < 16; j++) {
      buffer1[i * 16 + j] = buffer[j];
    }
  }

//file status, first block number, length and descriptor should be converted from unsigned char to int for further operations
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      if (j == 0 || j == 1 || j == 6 || j == 7) {
        DIR[i][j] = buffer1[k];
        k++;
      }else{
        DIR[i][j] = buffer1[k];
        k++;
      }
    }
  }  



  for (i = 0; i < 4; i++) {
    block_read(i + 5, buffer);
    for (j = 0; j < 16; j++) {
      buffer2[i * 16 + j] = buffer[j];
    }
  }

  k = 0;
  for (i = 0; i < 32; i++) {
    for (j = 0; j < 2; j++) {
      FAT[i][j] = (int)buffer2[k];
      k++;
    }
  }

  for(i = 0; i < 8; i++){
    DIR[i][7] = -1;
  }


  return 0;
}

/*dismount_fs() function:
 copy the metadata from memory to disk file and update the information;
 also reset DIR[][], FAT[][] and OFT[][] to orginal state after close the disk
 block 1-4: DIR[][] 
 block 5-8: FAT[][]*/
int dismount_fs(char *disk_name) {
  int i = 0, j = 0, k = 0, index = 0;
  unsigned char buff[16];

//every block can store the information of 2 files  
  for (i = 1; i < 5; i++){
    for (j = 0; j < 8; j++){
      buff[j] = DIR[index][j];
    }
    index++;
    for (j = 8; j < 16; j++){
      buff[j] = DIR[index][j - 8];
    }
    index++;
    block_write(i, buff);    
  }

//every block can store the informatin of 8 blocks
  index = 0;
  for (i = 5; i < 9; i++){
    k = 0;
    for (j = 0; j < 8; j++){
      buff[k] = FAT[index][0];
      //printf("status%d\n", FAT[index][0]);
      k++;
      buff[k] = FAT[index][1];
      //printf("block%d\n", FAT[index][1]);
      k++;
      index++;
    }    
    block_write(i, buff);  
  }
  
  close_disk();

//reset DIR[][], FAT[][] and OFT[][]   
  for (i = 0; i < 4; i++) {
    OFT[i][0] = 0;
    OFT[i][1] = -1;
    OFT[i][2] = 0;
  }
  
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      if(j == 7){
        DIR[i][j] = -1;
      }
      DIR[i][j] = 0;
    }
  }

  for (i = 0; i < 32; i++) {
    for (j = 0; j < 2; j++) {
      FAT[i][j] = 0;
    }
  }
  num_fildes = 0;
  num_file = 0;

  return 0;
}

/*fs_open() function:
 open a file with name and update relevant information*/
int fs_open(char *name) {
  int i = 0, j = 0, flag = 0, index = 0;
  char *p = name;
  char filename[5];

//search for the file with name and record the file's index in DIR[][]
  for (i = 0; i < 8; i++) {
    for(j = 0; j < 4; j++){
      filename[j] = DIR[i][2 + j];
    }
    filename[4] = '\0';
    if (strcmp(p, filename) == 0) {
      flag = 1;
      index = i;
      break;
    }
  }
  
//if there is no such file or the file is already open, return -1
  if (flag == 0) {
    return -1;
  }  

  if (DIR[index][7] > 0) {
    printf("Error: the file is already open\n");
    return -1;
  }  

  if (flag == 1) {
    num_fildes++;
  }

//if the 4 file descriptors has all been allocated, return -1  
  if (num_fildes > 4) {
    printf("Error: there are 4 file descriptors which are open right now\n");
    return -1;
  }  

//search for availble file descriptor using first-fit algorithm  
  for (i = 0; i < 4; i++) {
    if (OFT[i][0] == 0) {
      index_filedes = i;
      break;
    }
  }

//update the DIR[][] and OFT[][]
  DIR[index][7] = index_filedes;

  OFT[index_filedes][0] = 1;
  OFT[index_filedes][1] = index;
  OFT[index_filedes][2] = 0;

//decide the fisrt block for the file  
  if(DIR[index][1] == 0){
    for (i = 0; i < 32; i++) {
      if(FAT[i][0] == 0){
        index_block = i + 32;
      //printf("first free block %d\n", index_block);
        break;
      }
    }
  
//update the relevant information
    DIR[index][0] = 1;
    DIR[index][1] = index_block;
    //printf("%d\n", DIR[index][1]);
    DIR[index][6] = 0;
    DIR[index][7] = -1;

    FAT[index_block - 32][0] = 1;
    FAT[index_block - 32][1] = 0;
  }
  return index_filedes;
}

/*fs_close() function:
 reset the file descriptor to original state and update the relevant information*/
int fs_close(int fildes) {

//if file descriptor is not open or does not exist, return -1
  if (OFT[fildes][0] == 0 || fildes < 0 || fildes > 3) {
    return -1;
  }

//update DIR[][] and OFT[][]
  DIR[OFT[fildes][1]][7] = -1;
  OFT[fildes][0] = 0;
  OFT[fildes][1] = -1;
  OFT[fildes][2] = 0;

  num_fildes--;
  
  return 0;
}

/*fs_create() function:
 create a new file in the file system*/
int fs_create(char *name) {
  char *p = name;
  char filename[5];   
  int count = 0;
  int flag = 0;
  int index = -1; 
  int i = 0, j = 0, k = 2;
  
  for (i = 0; i < 8; i++) {
    for(j = 0; j < 4; j++){
      filename[j] = DIR[i][2 + j];
    }
    filename[4] = '\0';
    if(strcmp(p, filename) == 0){
      printf("Error: file already exist\n");
      return -1;
    }
  }

//search for an available directory using first-fit algorithm
  for (i = 0; i < 8; i++) {
      if (DIR[i][0] == 0) {
        index = i;
        break;
      }
  }

//check and record the file name
  while (*p != '\0') {
    count++;
    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
      DIR[index][k] = *p;  
      p++;
      k++;    
    }else{
        flag = 1;
        break;
    }        
  }
  num_file++;

//if the name is illegal or the number of files exceeds 8, return -1
  if (flag == 1 || count > 4 || num_file > 8) {
    printf("Failure: create a file\n");
    DIR[index][0] = 0;
    return -1;
  }  

  return 0; 
}

/*fs_delete() function:
 delete a file from the file system*/
int fs_delete(char *name) {
  char *p = name;  
  int flag = 0;
  int i = 0, j = 0, index = 0;
  int nextblock = 0;
  char filename[5];

//check if the file exists  
  for (i = 0; i < 8;i++) {
    for(j = 0; j < 4; j++){
      filename[j] = DIR[i][2 + j];
    }
    filename[4] = '\0';
    if(strcmp(p, filename) == 0){
       flag = 1;
       index = i;
       break;
    }
  } 

//if the file is open or does not exist, return -1 
  if (DIR[index][7] > -1 || flag == 0) {
    return -1;
  }
    
//free the corresponding blocks
  int m = DIR[index][6] / 16;
  int n = DIR[index][6] % 16;
  
  if(n != 0){
    m++;
  }
  
  nextblock = DIR[index][1] + i - 32;
  for (i = 0; i < m; i++) {
    FAT[nextblock][0] = 0;
    nextblock = FAT[nextblock][1];
    //printf("free block %d\n", FAT[DIR[index][1] + i - 32][1]);
    //printf("status %d\n", FAT[DIR[index][1] + i - 32][0]);
  }

//delete relevant informantion
  for (i = 0; i < 7; i++) {
    DIR[index][i] = 0;
  }
  
  DIR[index][7] = -1;
  
  num_file--;

  return 0;
}

/*fs_read() function:
 read nbyte bytes of data from the file referenced by fildes*/
int fs_read(int fildes, void *buf, size_t nbyte){
  int i = 0, j = 0;
  int nextblock = 0;
  unsigned char buffer[256];
 
//if fildes is not valid, return -1
  if(OFT[fildes][0] == 0){
    return -1;
  }

//calculate the number of blocks needed for the corresponding file
  int index = OFT[fildes][1];
  
  //printf("block%d\n", DIR[3][1]);
  int m = DIR[index][6] / 16;
  int n = DIR[index][6] % 16;  
  
  if(n != 0){
    m++;
  }

  block_read(DIR[index][1], buffer);
  nextblock = FAT[DIR[index][1] - 32][1];

//copy the data from disk file to a buffer in memory, 16 characters per copying  
  for(i = 0; i < m - 1; i++){
    block_read(nextblock, buffer + 16 * (i + 1));
    nextblock = FAT[nextblock - 32][1];
  }
//prevent from reading past the end of the file
  if (nbyte > DIR[index][6]) {
    nbyte = DIR[index][6];
  }
//copy the required characters to buf according to the offset and nbyte  
  memcpy(buf, buffer + OFT[fildes][2], nbyte);
  
  OFT[fildes][2] += nbyte;
   
  return 0;
}

/*fs_write() function:
 write nbyte bytes of data to the file referenced by fildes*/
int fs_write(int fildes, void *buf, size_t nbyte) {
  int i = 0, j = 0, currblock = 0, nextblock = 0;
  unsigned char buffer[16];
  unsigned char buffa[256];
  int index = 0;
  int m = 0, n = 0;

//check if fildes is valid
  if (OFT[fildes][0] == 0) {
    return -1;
  }

  if(nbyte > 512){
    return -1;
  }

  if(OFT[fildes][2] == 0){
    m = nbyte / 16;
    n = nbyte % 16;
    if(n != 0){
      m++;
    }

    index = OFT[fildes][1];
    DIR[index][6] = nbyte;

    memcpy(buffer, buf, 16);
    block_write(DIR[index][1], buffer);
    FAT[DIR[index][1] - 32][0] = 1; 
    currblock = DIR[index][1];
  
//copy data from buf to buffer, then use block_write() to copy data from buffer to the disk file  
    for (i = 0; i < m - 1; i++) {
      for (j = 0; j < 32; j++) {
        if (FAT[j][0] == 0) {
          nextblock = j + 32;
          break;
        }
      }
  
      if(nextblock == 0){
        printf("Error: the file system is full\n");
        return -1;
      }

      FAT[currblock - 32][1] = nextblock;
      currblock = nextblock;

      memcpy(buffer, buf + 16 * (i + 1), 16);
      block_write(currblock, buffer);

      FAT[currblock - 32][0] = 1; 
    }
  
    OFT[fildes][2] = DIR[index][6];
  }else if(OFT[fildes][2] == DIR[OFT[fildes][1]][6]){

      index = OFT[fildes][1];
      int left = DIR[index][6] % 16; 
      
      currblock = DIR[index][1];
      while(FAT[currblock - 32][1] >= 32){
        currblock = FAT[currblock - 32][1];
        
      }
      block_read(currblock, buffa);
      
      if(nbyte <= 16 - left && left != 0){
        memcpy(buffa + left, buf, nbyte);
        block_write(currblock, buffa);
        OFT[fildes][2] += nbyte;
        DIR[index][6] += nbyte;  
      }else{
        if(left != 0){
          memcpy(buffa + left, buf, 16 - left);
          block_write(currblock, buffa);
          m = (nbyte - (16 - left)) / 16;
          n = (nbyte - (16 - left)) % 16;
          if(n != 0){
            m++;
          }
          for (i = 0; i < m; i++) {
            for (j = 0; j < 32; j++) {
              if (FAT[j][0] == 0) {
                nextblock = j + 32;
                break;
              }
            }
  
            if(nextblock == 0){
              printf("Error: the file system is full\n");
              return -1;
            }

            FAT[currblock - 32][1] = nextblock;
            currblock = nextblock;

            memcpy(buffer, buf + 16 - left + 16 * i, 16);
            block_write(currblock, buffer);

            FAT[currblock - 32][0] = 1; 
          }
        }else{
          m = nbyte / 16;
          n = nbyte % 16;
          if(n != 0){
            m++;
          }
        }

        for (i = 0; i < m; i++) {
          for (j = 0; j < 32; j++) {
            if (FAT[j][0] == 0) {
              nextblock = j + 32;
              break;
            }
          }
  
          if(nextblock == 0){
            printf("Error: the file system is full\n");
            return -1;
          }

          FAT[currblock - 32][1] = nextblock;
          currblock = nextblock;

          memcpy(buffer, buf + 16 * i, 16);
          block_write(currblock, buffer);

          FAT[currblock - 32][0] = 1; 
        }
        OFT[fildes][2] += nbyte;
        DIR[index][6] += nbyte;                 
      }  
  }else{
      index = OFT[fildes][1];
      currblock = DIR[index][1];

      int p = OFT[fildes][2] / 16;
      int q = OFT[fildes][2] % 16;
      if(q != 0){
        p++;
      }

      int x = DIR[index][6] / 16;
      int y = DIR[index][6] % 16;
      if(y != 0){
        x++;
      } 

//1   
      if(q != 0 && nbyte <= (16 - q)){
        for(i = 0; i < p - 1; i++){
          currblock = FAT[currblock - 32][1];
        }
        printf("%d\n", currblock);
        block_read(currblock, buffer);
        memcpy(buffer + q, buf, nbyte);
        block_write(currblock, buffer);
        OFT[fildes][2] += nbyte;
      } 

      if(q != 0 && nbyte > 16 - q && nbyte <= 16 * x - OFT[fildes][2]){
        for(i = 0; i < p - 1; i++){
          currblock = FAT[currblock - 32][1];
        }

        block_read(currblock, buffer);
        memcpy(buffer + q, buf, 16 - q);
        block_write(currblock, buffer);

        m = (nbyte - 16 + q) / 16;
        n = (nbyte - 16 + q) % 16;
        
        if(n != 0){
          m++;
        }

        for(i = 0; i < m - 1; i++){
          currblock = FAT[currblock - 32][1];
          block_read(currblock, buffer);
          memcpy(buffer, buf, 16);
          block_write(currblock, buffer);
        }

        currblock = FAT[currblock][1];

        block_read(currblock, buffer);
        memcpy(buffer, buf, nbyte - 16 + q - 16 * (m - 1));
        block_write(currblock, buffer);

        OFT[fildes][2] += nbyte;
      } 
//2
      if(q == 0 && nbyte <= 16){
        for(i = 0; i < p + 1; i++){
          currblock = FAT[currblock - 32][1];
        }
        block_read(currblock, buffer);
        memcpy(buffer, buf, nbyte);
        block_write(currblock, buffer);
        OFT[fildes][2] += nbyte;
      }
//3
      if(q == 0 && nbyte > 16 && nbyte <= 16 * (x - p)){
        for(i = 0; i < p; i++){
          currblock = FAT[currblock - 32][1];
        }

        m = nbyte / 16;
        n = nbyte % 16;
        
        if(n != 0){
          m++;
        }

        for(i = 0; i < m - 1; i++){
          block_read(currblock, buffer);
          memcpy(buffer, buf + 16 * i, 16);
          block_write(currblock, buffer);
          currblock = FAT[currblock - 32][1];
        }
        
        block_read(currblock, buffer);
        memcpy(buffer, buf + 16 * (m - 1), nbyte - 16 * (m - 1));
        block_write(currblock, buffer);
        currblock = FAT[currblock - 32][1];        
        OFT[fildes][2] += nbyte;
      }
//4
      if(q == 0 && nbyte > 16 * (x - p)){
        index = OFT[fildes][1];
        currblock = DIR[index][1];
        for(i = 0; i < p; i++){
          currblock = FAT[currblock - 32][1];
        }

        m = nbyte / 16;
        n = nbyte % 16;
        if(n != 0){
          m++;
        }

        for(i = 0; i < x - p ; i++){
          block_read(currblock, buffer);
          
          memcpy(buffer, buf + 16 * i, 16);
          block_write(currblock, buffer);
          currblock = FAT[currblock - 32][1];
        }

        for(i = 0; i < m - (x - p); i++){
          for (j = 0; j < 32; j++) {
            if (FAT[j][0] == 0) {
              nextblock = j + 32;
              
              break;
            }
          }
         
          if(nextblock == 0){
            printf("Error: the file system is full\n");
            return -1;
          }

          FAT[currblock - 32][1] = nextblock;
          currblock = nextblock;
          printf("%d\n", currblock);
          if(i == m - (x - p) - 1){
            memcpy(buffer, buf + 16 * (x - p + i), nbyte - (m - 1) * 16);
          }else{
            memcpy(buffer, buf + 16 * (x - p + i), 16);
          }
          block_write(currblock, buffer);

          FAT[currblock - 32][0] = 1; 
        }
        
        OFT[fildes][2] += nbyte;

        DIR[index][6] = OFT[fildes][2];
      }  
  }
   
  return 0;
}

/*fs_get_filesize() function*/
int fs_get_filesize(int fildes) {

//check if fildes is valid
  if(OFT[fildes][0] == 0){
    return -1;
  }

  return DIR[OFT[fildes][1]][6];   
}

/*fs_lseek() function*/
int fs_lseek(int fildes, off_t offset){
  int pos = 0;
  pos = OFT[fildes][2] + offset;

//if pos is out of bound, return -1
  if (pos < 0 || pos > DIR[OFT[fildes][1]][6]) {
    return -1;
  }

  OFT[fildes][2] = pos;

  return 0;
}

/*fs_truncate() function*/
int fs_truncate(int fildes, off_t length){
  int index = OFT[fildes][1];
  int i = 0;
  int nextblock = 0;

  if (OFT[fildes][0] == 0 || length > DIR[index][6]) {
    return -1;
  }
  
//calculate the number of blocks before truncating
  int m = DIR[index][6] / 16;
  int n = DIR[index][6] % 16;  
  
  if(n != 0){
    m++;
  }

  DIR[index][6] = length;
  OFT[fildes][2] = 0;

//calculate the number of blocks after truncating  
  int p = DIR[index][6] / 16;
  int q = DIR[index][6] % 16;  
  
  if(q != 0){
    p++;
  }
  nextblock = DIR[index][1] + p;
//free the extra blocks
  for (i = 0; i < m - p; i++) {
    FAT[nextblock -32][0] = 0;
    //printf("free block %d\n", nextblock);printf("status %d\n", FAT[nextblock - 32][0]);
    nextblock = FAT[nextblock - 32][1];
    
  }

  return 0;
}
