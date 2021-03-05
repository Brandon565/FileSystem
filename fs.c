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
#define BLOCK_MAX_BYTES 4096

struct __attribute__ ((packed)) superBlock {
	char signature[8];
	int16_t totalBlocks;
	int16_t rootDirectory;
	int16_t dataBlock;
	int16_t dataBlockAmt;
	int8_t fatBlockAmt;
	int8_t padding[4079];
};

struct __attribute__ ((packed)) fileEntry{
	char fileName[16];
	int32_t fileSize;
	uint16_t dataBlockBegin;
	int8_t padding[10];
};

struct __attribute__ ((packed)) fileDescript{
	int offset;
	char* name;
	int32_t fileSize;

};

struct superBlock* mounted;
struct fileEntry* root;		    //root is an array of file structs of size 128
uint16_t* fatBuf;
struct fileDescript* openFiles;

int openFile = 0;
int numberOpenFiles = 0;
int fdArray[32] = {0}; 

int fs_mount(const char *diskname)
{

	char checkSig[9] = "ECS150FS\0";

	if(block_disk_open(diskname) == -1){
		return -1;
	}

	mounted = (struct superBlock*)malloc(sizeof(struct superBlock));
	root = (struct fileEntry*)malloc(FS_FILE_MAX_COUNT * sizeof(struct fileEntry));
	openFiles = (struct fileDescript*)malloc(sizeof(struct fileDescript) * FS_OPEN_MAX_COUNT);

	block_read(0, mounted);
	mounted->signature[8] = '\0';

	block_read(mounted->rootDirectory, root);

	if(strcmp(mounted->signature, checkSig) != 0){
		return -1;
	}

	if( block_read(mounted->rootDirectory, root) == -1){
		return -1;
	}

	int c = 0;
	fatBuf = (uint16_t*)malloc(sizeof(uint16_t) * FAT_MAX_SIZE);
	for (int i = 0; i < 1 + floor((mounted->dataBlockAmt-1)/FAT_MAX_SIZE); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < FAT_MAX_SIZE; j++)
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
	if(openFile == 0){
		return -1;
	}
	free(mounted);
	free(root);

	if (block_disk_close() == -1) {
        	return -1;
    	}
	openFile = 0;

	return 0;
}


int fs_info(void)
{
	if(openFile == 0)
	{
		return -1;
	}

	block_read(0, mounted);
	int rdirFree = FS_FILE_MAX_COUNT;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if ((int)root[i].fileName[0] != 0) {
			rdirFree--;
 		}
  	}

	int fatFree = mounted->dataBlockAmt;
	int count = 0;
	fatBuf = (uint16_t*)malloc(sizeof(uint16_t) * FAT_MAX_SIZE);
	for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/FAT_MAX_SIZE); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < FAT_MAX_SIZE; j++)
		{
			count++;
			if (fatBuf[j] != 0) {
  				fatFree--;
			}
			if (count == mounted->dataBlockAmt) {
        			break;
 			}
		}

	}

	free(fatBuf);

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", mounted->totalBlocks);
	printf("fat_blk_count=%d\n", mounted->fatBlockAmt);
	printf("rdir_blk=%d\n", mounted->rootDirectory);
	printf("data_blk=%d\n", mounted->dataBlock);
	printf("data_blk_count=%d\n", mounted->dataBlockAmt);
	printf("fat_free_ratio=%d/%d\n",fatFree, mounted->dataBlockAmt);
	printf("rdir_free_ratio=%d/128\n", rdirFree);

	return 0;
}


int fs_create(const char *filename)
{
	if(openFile == 0 || strlen(filename) > FS_FILENAME_LEN - 1){
		return -1;
	}
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        	if (root[i].fileName != NULL && !strcmp(root[i].fileName, filename)) {
            		return -1;
        	}
   	}

	int rootIndex = -1;
  	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    		if ((int)root[i].fileName[0] == 0) {
      			rootIndex = i;
      			strcpy(root[i].fileName, filename);
      			break;
    		}
 	}

	root[rootIndex].dataBlockBegin = FAT_EOC;
	root[rootIndex].fileSize = 0;

	block_write(mounted->rootDirectory, root);

  	return 0;
}


int fs_delete(const char *filename)
{
  	//BEGIN ERROR HANDLING/INPUT VALIDATION
	int fileExistsFlag = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
  		if (root[i].fileName != NULL && !strcmp(root[i].fileName, filename)) {
  			fileExistsFlag = 1;
  		}
	}

	for (int i = 0; i < 32; i++) {
		if (openFiles[i].name != NULL && !strcmp(openFiles[i].name, filename)) {
       			return -1;
     		}
  	}

  	if (!fileExistsFlag) {
  		return -1;
  	}
  	//END ERROR HANDLING/INPUT VALIDATION

  	// find the root index of the file
  	int rootIndex = -1;
  	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(root[i].fileName,filename)) {
			rootIndex = i;
			root[i].fileName[0] = '\0';
			root[i].fileSize = 0;
			break;
		}
  	}

  	// if the file has no data blocks we are done
	if (root[rootIndex].dataBlockBegin == FAT_EOC) {
		fflush(stdout);
		block_write(mounted->rootDirectory, root);
		return 0;
	}

	int count = 0;
	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE * sizeof(uint16_t));

  	for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/FAT_MAX_SIZE); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < FAT_MAX_SIZE; j++) {
      		// if we are on the right fat spot
			if (count == root[rootIndex].dataBlockBegin) {
        		// if this is NOT the last entry in fat for file
  				if (fatBuf[j] != FAT_EOC) {
  					root[rootIndex].dataBlockBegin = fatBuf[j];
   					fatBuf[j] = 0;
  					block_write(1 + i, fatBuf);
  					count = 0;
  					i = -1;
  					j = FAT_MAX_SIZE;
  					continue;
  				} else { // if this is the last entry in fat for file
  					fatBuf[j] = 0;
  					block_write(1 + i, fatBuf);
  					root[rootIndex].dataBlockBegin = FAT_EOC;
  					i = 10;
  					break;
  				}
			}

     			 // we reached the end
			if (count == mounted->dataBlockAmt) {
  				break;
			}
  		count++;
		}
	}

  	// change the root dir to show deletion
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
			printf("file: %s, size: %d, data_blk: %d\n", root[i].fileName, root[i].fileSize, root[i].dataBlockBegin);
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
		return -1;
	}
	if(numberOpenFiles == FS_OPEN_MAX_COUNT)
	{
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
			break;
		}
		fdVal++;
	}

	numberOpenFiles ++;
	fdArray[fdVal] = 1;

	return fdVal;  //return filedescriptor of opened file

}

int fs_close(int fd)
{
	if(fd > 32 || fdArray[fd] == 0)
	{
		return -1;
	}
	//set all bytes for the given file to 0 
	fdArray[fd] = 0;
	memset(&openFiles[fd], 0, sizeof(struct fileDescript));
	openFiles[fd].offset = -1;

	numberOpenFiles --;
	return 0;

}

//print file size for file with filedescriptor fd  
int fs_stat(int fd)
{
	
	if(openFile == 0 || fd > 31 || fdArray[fd] == 0)
		return -1;

	printf("Filesize: %d", openFiles[fd].fileSize);

	return openFiles[fd].fileSize;
}

//set offset within file struct to user input
int fs_lseek(int fd, size_t offset)
{


	if(openFile == 0 || fd > 31 || fdArray[fd] == 0)
		return -1;

	if((int32_t)offset > openFiles[fd].fileSize)
	{ //if the offset is larger than the fileSize input
		return -1;
	}

	openFiles[fd].offset = offset;

	return 0;
}

// return the index of data block where offset of @fd is
int offsetBlock(int fd) {
	// find start data block for file
	uint16_t prevBlock = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(openFiles[fd].name, root[i].fileName)) {
			prevBlock = root[i].dataBlockBegin;
			break;
		}
	}

	if (prevBlock == FAT_EOC || prevBlock == 0) {
		return -1;
	}

	int count = 0;

	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE * sizeof(uint16_t));
	block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
	int nextBlock = fatBuf[prevBlock%FAT_MAX_SIZE];
	count += BLOCK_MAX_BYTES;

	while(count <= openFiles[fd].offset) {
    	// if there isnt a next data block
		if (nextBlock == FAT_EOC) {
			return -1;
		}
		block_read(1 + floor(nextBlock/FAT_MAX_SIZE), fatBuf);
		prevBlock = nextBlock;
		nextBlock = fatBuf[nextBlock%FAT_MAX_SIZE];
		count += BLOCK_MAX_BYTES;
	}

	if (prevBlock == FAT_EOC || prevBlock == 0) {
		return -1;
	}

	return prevBlock;

}

// set empty fat spot to FAT_EOC and return its index
int find_fat_block()
{
	int count = 0;
	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE* sizeof(uint16_t));
	for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/FAT_MAX_SIZE); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < FAT_MAX_SIZE; j++) {
			count++;
			if (fatBuf[j] == 0) {
				fatBuf[j] = FAT_EOC;
  				block_write(1 + i, fatBuf);
				return  i * FAT_MAX_SIZE + j;
			}
			if (count == mounted->dataBlockAmt) {
				return -1;
			}
		}
	}

	return -1;
}

int fs_write(int fd, void *buf, size_t count)
{
	int sizeInc = openFiles[fd].offset;
	// error handling
		
	if(openFiles[fd].offset == -1){
		return -1;
	}	

	if (buf == NULL ||  openFile == 0 || fd > 31 || fdArray[fd] == 0) {
		return -1;
	}

	if ((int)count == 0) {
		return 0;
	}

	int rootIndex = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(root[i].fileName, openFiles[fd].name)) {
			rootIndex = i;
			break;
		}
	}

	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE * sizeof(uint16_t));
	int prevBlock = root[rootIndex].dataBlockBegin;

	int bytesWrote = 0;
	int currBlock = offsetBlock(fd);


	// if there is no data block assigned
	if (currBlock == -1) {
		currBlock = find_fat_block();
		// if there are no free fat spots, cannot write
		if (currBlock == -1) {
			return bytesWrote;
		}

		if (root[rootIndex].dataBlockBegin == FAT_EOC) {
			root[rootIndex].dataBlockBegin = currBlock;
			block_write(mounted->rootDirectory, root);
		} else {
			block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
			int nextBlock = fatBuf[prevBlock%FAT_MAX_SIZE];

			while(fatBuf[prevBlock%FAT_MAX_SIZE] != 0 && fatBuf[prevBlock%FAT_MAX_SIZE] != FAT_EOC) {
				block_read(1 + floor(nextBlock/FAT_MAX_SIZE), fatBuf);
				prevBlock = nextBlock;
				nextBlock = fatBuf[nextBlock%FAT_MAX_SIZE];
			}

			fatBuf[prevBlock%FAT_MAX_SIZE] = currBlock;
			block_write(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
		}
		prevBlock = currBlock;
		block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
	}

	int currOffset = openFiles[fd].offset % BLOCK_MAX_BYTES;
	uint8_t * blockBuf = (uint8_t* )malloc(BLOCK_MAX_BYTES* sizeof(uint8_t));

	while (bytesWrote < (int)count) {
		block_read(currBlock + mounted->dataBlock, blockBuf);

		// if we can write the entirety of the remaining block
		if ((int) count >= bytesWrote + BLOCK_MAX_BYTES - currOffset) {
			memcpy(blockBuf + currOffset, buf + bytesWrote, BLOCK_MAX_BYTES - currOffset);
			bytesWrote += BLOCK_MAX_BYTES - currOffset;
			openFiles[fd].offset += BLOCK_MAX_BYTES - currOffset;
		} else {
			memcpy(blockBuf + currOffset, buf + bytesWrote, count - bytesWrote);
			openFiles[fd].offset += count - bytesWrote;
			bytesWrote += count - bytesWrote;
		}

		block_write(currBlock + mounted->dataBlock, blockBuf);


		currBlock = offsetBlock(fd);
		// if there is no data block assigned
		if (currBlock == -1 && bytesWrote < (int)count) {
			currBlock = find_fat_block();
			block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
			// if there are no free fat spots, cannot write
			if (currBlock == -1) {
				sizeInc = bytesWrote - (openFiles[fd].fileSize - sizeInc);
				if (sizeInc > 0) {
					root[rootIndex].fileSize += bytesWrote;
					block_write(mounted->rootDirectory, root);
					openFiles[fd].fileSize += bytesWrote;
				}
				return bytesWrote;
			}

			fatBuf[prevBlock%FAT_MAX_SIZE] = currBlock;
			block_write(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
			prevBlock = currBlock;
			block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
		}
		currOffset = 0;
	}

	sizeInc = bytesWrote - (openFiles[fd].fileSize - sizeInc);
	if (sizeInc > 0) {
		root[rootIndex].fileSize += bytesWrote;
		block_write(mounted->rootDirectory, root);
		openFiles[fd].fileSize += bytesWrote;
	}

	return bytesWrote;
}


int fs_read(int fd, void *buf, size_t count)
{
	//error handling 
	if (buf == NULL ||  openFile == 0 || fd > 31 || fdArray[fd] == 0) {
		return -1;
	}

	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE * sizeof(uint16_t));
	int i = 0;
	int stopIndex;
	for (i = 0; i < 1 + floor(mounted->fatBlockAmt/FAT_MAX_SIZE) ; ++i)
	{
		stopIndex = block_read(i + 1, fatBuf);
		if (stopIndex == FAT_EOC)
			break;
	}

	int block = BLOCK_MAX_BYTES; 
	int blockCount = 1 + floor(count/block);

	int initOffset = openFiles[fd].offset;

	int currentBlock = offsetBlock(fd);
	int offsetCount = 1;
	//shift offset is it is located outside the current block
	while(initOffset >= block)
	{
		initOffset -= block;
		offsetCount++;
	}

	//add an additional block to read if input necessary
	if( (initOffset + (int)count) > (block * offsetCount))
	{
		blockCount ++;
	}

	char* bounce = (char*)malloc(block * sizeof(char) * blockCount); //temporary bounce buffer

	int blockNum;			
	int bounceBuf = 0;
	int endLoop = currentBlock + blockCount;
	int blockMult;
	//loop through each block of the file and read all the full blocks into the bounce buffer
	for(blockNum = currentBlock; blockNum <= endLoop ; blockNum++)
	{
		if(currentBlock == FAT_EOC){
			break;
		}

		blockMult = block * bounceBuf;	  //track number of bytes being read
		block_read(currentBlock + mounted->dataBlock , bounce + blockMult);
		currentBlock = fatBuf[currentBlock];
		bounceBuf++;
	}
	//truncate the blocks read based on the given offset and number of bytes read 
	memcpy(buf, bounce + initOffset, (int)count);

	int readLength = strlen(buf);
	openFiles[fd].offset += readLength;

	return readLength;

}
