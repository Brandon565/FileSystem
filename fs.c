#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 65535
#define FAT_MAX_SIZE 2048

struct __attribute__ ((packed)) superBlock {
	char signature[8];
	int16_t totalBlocks;
	int16_t rootDirectory;
	int16_t dataBlock;
	int16_t dataBlockAmt;
	int8_t fatBlockAmt;
	int8_t padding[4079];
};

struct __attribute__ ((packed)) fileEntry {
	char fileName[16];
	int32_t fileSize;
	int16_t dataBlockBegin;		//first fat block of the fat chain 
	int8_t padding[10];
};

struct __attribute__ ((packed)) fileDescript {
	int offset;
	char* name;
	int32_t fileSize;
	int16_t dataBlockBegin;

};



struct superBlock* mounted;
struct fileEntry* root;		    //root is an array of file structs of size 128
uint16_t* fatBuf;
struct fileDescript* openFiles;

int openFile = 0;
int numberOpenFiles = 0;

int fs_mount(const char *diskname)
{

	char checkSig[9] = "ECS150FS\0";

	if(block_disk_open(diskname) == -1){
		printf("Invalid open\n");
		return -1;
	}

	mounted = (struct superBlock*)malloc(sizeof(struct superBlock));
	root = (struct fileEntry*)malloc(128 * sizeof(struct fileEntry));
	openFiles = (struct fileDescript*)malloc(sizeof(struct fileDescript) * FS_OPEN_MAX_COUNT);

	block_read(0, mounted);
	mounted->signature[8] = '\0';

	if(strcmp(mounted->signature, checkSig) != 0){
		printf("Wrong Signature\n");
		return -1;
	}

	if(block_read(mounted->rootDirectory, root) == -1){
		printf("Root Directory Error");
		return -1;
	}

	int c = 0;
	fatBuf = (uint16_t*)malloc(sizeof(uint16_t) * 2048);
	for (int i = 0; i < 1 + floor((mounted->dataBlockAmt-1)/2048); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < 2048; j++)
		{
			c++;
			if (c == mounted->dataBlockAmt) {
        			break;
 			}
		}
	}

	openFile = 1;

	return 0;

}

int fs_umount(void)
{
	block_write(mounted->rootDirectory, root);
	
	free(mounted);
	free(root);
	
	block_disk_close();
	openFile = 1;

	return 0;
}


int fs_info(void)
{
	int rdirFree = 128;
	for (int i = 0; i < 128; i++) {
		if ((int)root[i].fileName[0] != 0) {
			rdirFree--;
			printf("File found at spot: %d\n", i);
 		}
  }
	int fatFree = mounted->dataBlockAmt;
	int count = 0;
	fatBuf = (uint16_t*)malloc(sizeof(uint16_t) * 2048);
	for (int i = 0; i < 1 + floor((mounted->dataBlockAmt-1)/2048); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < 2048; j++)
		{
			count++;
			//printf("%d\n",fatBuf[j]);
			if (fatBuf[j] != 0) {
  			fatFree--;
				printf("FAT: %d, I: %d, J: %d\n", fatBuf[j], i, j);
			}
			if (count == mounted->dataBlockAmt) {
        			break;
 			}
		}
		/*for (int k = 0; k < 2048; k++) {
			printf("FAT: %d, ", fatBuf[k]);
		}*/

	}

	free(fatBuf);

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", mounted->totalBlocks);
	printf("fat_blk_count%d\n", mounted->fatBlockAmt);
	printf("rdir_blk=%d\n", mounted->rootDirectory);
	printf("data_blk=%d\n", mounted->dataBlock);
	printf("data_blk_count=%d\n", mounted->dataBlockAmt);
	printf("fat_free_ratio=%d/%d\n",fatFree, mounted->dataBlockAmt);
	printf("rdir_free_ratio=%d/128\n", rdirFree);

	return -1;
}

int fs_create(const char *filename)
{
	int rootIndex = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if ((int)root[i].fileName[0] == 0) {
			rootIndex = i;
			strcpy(root[i].fileName, filename);
			break;
    		}
	}

	int count = 0;
  	uint16_t * fatBuf = (uint16_t* )malloc(2048* sizeof(uint16_t));
  	for (int i = 0; i < ceil(mounted->dataBlockAmt/2048); i++) {
    		block_read(1 + i, fatBuf);
    			for (int j = 0; j < 2048; j++) {
      				count++;
      				if (fatBuf[j] == 0) {
        				fatBuf[j] = 65535;
        				block_write(1 + i, fatBuf);
					printf("writing to block: %d\n", 1+i);
        				root[rootIndex].dataBlockBegin = i * 2048 + j;
        				i = 10;
        				break;
      				}
      				if (count == mounted->dataBlockAmt) {
        				printf("we ran out of spots\n");
        				break;
      				}
    			}
  	}
  	root[rootIndex].fileSize = 0;
	
	block_write(mounted->rootDirectory, root);
	printf("writing to block2: %d\n", mounted->rootDirectory);

  	return 0;
}

int fs_delete(const char *filename)
{

	int rootIndex = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(root[i].fileName,filename)) {
      			rootIndex = i;
      			root[i].fileName[0] = '\0';
      			root[i].fileSize = 0;
      			break;
    		}
  	}

  	int count = 0;
  	uint16_t * fatBuf = (uint16_t* )malloc(2048* sizeof(uint16_t));
  	for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/2048); i++) {
  		block_read(1 + i, fatBuf);
    		for (int j = 0; j < 2048; j++) {
      			if (count == root[rootIndex].dataBlockBegin) {
        			if (fatBuf[j] != 65535) {
         				root[rootIndex].dataBlockBegin = fatBuf[j];
          				fatBuf[j] = 0;
          				block_write(1 + i, fatBuf);
          				count = 0;
          				i = 0;
          				j = 0;
          				break;
        			} else {
          				fatBuf[j] = 0;
          				block_write(1 + i, fatBuf);
          				root[rootIndex].dataBlockBegin = 0;
          				i = 10;
          				break;
        			}
      			}
	
      			if (count == mounted->dataBlockAmt) {
        			printf("we ran out of spots\n");
        			break;
      			}
      			count++;
    		}
  	}
	
	block_write(mounted->rootDirectory, root);

  	return 0;
}

int fs_ls(void)
{
	if (openFile == 0) {
		return -1;
	}

	printf("FS Ls:\n");

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if ((int)root[i].fileName[0] != 0) {
			printf("Filename: %s, Filesize: %d\n", root[i].fileName, root[i].fileSize);
			//break;
    		}
  	}

	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) 
	{ 					//print all openFiles 
		if (openFiles[i].name != NULL) 
		{
			printf("OPENFile: %s ", openFiles[i].name);
			printf("FD: %d \n", i);
    		}
  	}


  	return 0;
}

int fs_open(const char *filename)
{
	if (filename == NULL){
		return -1;	
	}

	int i;	 //index for files in root directory 
	int found = 0;
	for (i = 0; i < FS_FILE_MAX_COUNT; i++) //loop through files in the root directory
	{
		if(strcmp(root[i].fileName, filename) == 0) //find the filename in the list of root directory
		{
			found = 1;
			break;
		}
	}
	if(found == 0)	//check if file is not found in the root directory 
	{
		printf("File not found");
		return -1;
	}
	if(numberOpenFiles == FS_OPEN_MAX_COUNT)
	{
		printf("Max number of files open");
		return -1;
	}
	
	int fdVal = 0;
	while(fdVal < FS_OPEN_MAX_COUNT )	//loop through files in the openFiles array and find an empty spot 
	{
		if(openFiles[fdVal].name == NULL)
		{
			openFiles[fdVal].name = root[i].fileName;	//name added to openFiles, indexed by fileDescriptor
			openFiles[fdVal].offset = 0;			//offset set to start of file
			openFiles[fdVal].fileSize = root[i].fileSize;
			openFiles[fdVal].dataBlockBegin = root[i].dataBlockBegin;	
			break;
		}
		fdVal++;
	}

	numberOpenFiles ++;
	
	return fdVal;  //return filedescriptor of opened file

}

int fs_close(int fd)
{
	if(fd > numberOpenFiles)
	{
		return -1;
	}

	memset(&openFiles[fd], 0, sizeof(struct fileDescript));  

	numberOpenFiles --;	
	return 0;

}

int fs_stat(int fd)
{
	if(openFiles == NULL || fd > numberOpenFiles || openFiles[fd].name == NULL)
		return -1;

	printf("Filesize: %d", openFiles[fd].fileSize);

	return openFiles[fd].fileSize;
}

int fs_lseek(int fd, size_t offset)
{
	if(openFiles == NULL || fd > numberOpenFiles || openFiles[fd].name == NULL)
		return -1;

	if((int32_t)offset > openFiles[fd].fileSize)
	{
		printf("Offset out of file bounds");
		return -1;
	}
	openFiles[fd].offset = offset;

	return 0;
}

//Helper Functions
//1. Returns index based on offset
//2. Allocates new data block and link it to chain (first-fit strategy)
/*

// return index next available data block
int find_fat_block()
{
	int count = 0;
	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE* sizeof(uint16_t));
	for (int i = 0; i < ceil(mounted->dataBlockAmt/FAT_MAX_SIZE); i++) {
 		block_read(1 + i, fatBuf);
		for (int j = 0; j < FAT_MAX_SIZE; j++) 
		{
			count++;
			if (fatBuf[j] == 0) {
 				return  i * FAT_MAX_SIZE + j;
			}
			if (count == mounted->dataBlockAmt) {
				printf("we ran out of spots\n");
				return -1;
			}
		}
   	}
}
*/

// return the index of data block where offset of @fd is
int offsetBlock(int fd) {
	// find start data block for file

	int prevBlock = -1;
	
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(openFiles[fd].name, root[i].fileName)) {
			prevBlock = root[i].dataBlockBegin;
			break;
		}
	}
	

	if (prevBlock == FAT_EOC) {
		return -1;
	}

	int count = 0;

	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE * sizeof(uint16_t));
	block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
	int nextBlock = fatBuf[prevBlock % FAT_MAX_SIZE];
	count += 4096;
	fflush(stdout);
	
	while(count < openFiles[fd].offset) 
	{
		block_read(1 + floor(nextBlock/FAT_MAX_SIZE), fatBuf);
		prevBlock = nextBlock;
		nextBlock = fatBuf[nextBlock%FAT_MAX_SIZE];
		count += 4096;
	}

	return prevBlock;

}


/*
int fs_write(int fd, void *buf, size_t count)
{
	return 0;
}
*/

int fs_read(int fd, void *buf, size_t count)
{
	if(openFiles == NULL || fd > numberOpenFiles || openFiles[fd].name == NULL)
		return -1;

	int block = 4096;
	int blockMult;

	int blockCount = 1 +floor(count/block);   
  	printf("BLOCK COUNT: %d\n", blockCount);
	
	char* bounce = (char*)malloc(block * sizeof(char) * blockCount *blockCount );		  //temporary bounce buffer to store 
	int initOffset = openFiles[fd].offset;
	//int currentBlock = openFiles[fd].dataBlockBegin;
	printf("FILE NAME: %s \n", openFiles[fd].name);
	
	int currentBlock = offsetBlock(fd); 
	while(initOffset > block) 
	{
		initOffset -= block;
	}
	printf("Current Block: %d\n", currentBlock);
	printf("Offset Location: %d\n", initOffset);
	
	int blockNum;		//set block number to the starting block to read
	int bounceBuf = 0; 
	int endLoop = currentBlock + blockCount + 1;
	for(blockNum = currentBlock; blockNum < endLoop ; blockNum++)
	{
		printf("Index: %d | ", blockNum);
		printf("END: %d | ", endLoop);
		printf("Current Block: %d | ", currentBlock);
		printf("Bounce Buf: %d\n", bounceBuf);

		blockMult = block * bounceBuf; 		//track number of bytes being read
		block_read(blockNum + mounted->dataBlock , bounce + blockMult);	
		currentBlock = fatBuf[currentBlock];
		bounceBuf++;	
	}
	
	//All of blocks within the range are read to bounce buffer
	//printf("BUFFER: %s\n", bounce);

	memcpy(buf, bounce + initOffset, count);

	//printf("buf: %s\n", (char*)buf);
	
	int readLength = strlen(buf);	
	openFiles[fd].offset += readLength;

	printf("OFFSET: %d\n", initOffset);
	return readLength;
}

