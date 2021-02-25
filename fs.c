#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "disk.h"
#include "fs.h"

struct superBlock {
	char signature[8];
	int16_t totalBlocks;
	int16_t rootDirectory;
	int16_t dataBlock;
	int16_t dataBlockAmt;
	int8_t fatBlockAmt;
	int8_t padding[4079];
};

struct fileEntry{
	char fileName[16];
	int32_t fileSize;
	int16_t dataBlockBegin;
	int8_t padding[10];
};

struct fileDescript{
	int offset;
	char* name;
	int32_t fileSize;

};


struct superBlock* mounted;
struct fileEntry* root;		    //root is an array of file structs of size 128
//uint16_t* fatBlock;
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

	//fatBlock = (int16_t*)malloc(sizeof(int16_t) * 4096 * mounted->fatBlockAmt);
	//fatBlock[0] = -1;
	//for(int i = 1; i <= mounted->fatBlockAmt; i++) {
		//block_read(i, &fatBlock[i]);
		//printf("Index: %d  FAT: %d\n", i,fatBlock[i]);

	//}
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
 		}
  	}
	int fatFree = mounted->dataBlockAmt;
	int count = 0;
	fatBuf = (uint16_t*)malloc(sizeof(uint16_t) * 2048);
	for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/2048); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < 2048; j++)
		{
      			count++;
      			//printf("%d\n",fatBuf[j]);
      			if (fatBuf[j] != 0)
			{
        			fatFree--;
      			}
			if (count == mounted->dataBlockAmt)
			{
        			break;
     			}
		}
		if (i == 0) {
			int k;
			for (k = 0; k < 20; k++) {
				printf("FAT: %d\n", fatBuf[k]);
			}
		}
		//printf("Index: %d , FAT: %d\n", fatBuf[k]);

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
		if((int)root[i].fileName[0] == 0) {
			rootIndex = i;
			strcpy(root[i].fileName, filename);
      			break;
    		}
  	}

	int count = 0;
	uint16_t * fatBuf = (uint16_t* )malloc(2048* sizeof(uint16_t));
	for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/2048); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < 2048; j++) {
			count++;
			if (fatBuf[j] == 0) {
        			fatBuf[j] = 65535;
        			block_write(1 + i, fatBuf);
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
	
	//block_write(mounted->rootDirectory, root);
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


/*
int fs_write(int fd, void *buf, size_t count)
{
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	return 0;
}
*/
