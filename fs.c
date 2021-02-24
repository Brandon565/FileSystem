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

struct superBlock* mounted; 
struct fileEntry* root;		    //root is an array of file structs of size 128	
//uint16_t* fatBlock;
uint16_t* fatBuf;
int openFile = 0;

int fs_mount(const char *diskname)
{
	
	char checkSig[9] = "ECS150FS\0";
	
	if(block_disk_open(diskname) == -1){
		printf("Invalid open\n");
		return -1;
	}

	mounted = (struct superBlock*)malloc(sizeof(struct superBlock));
	root = (struct fileEntry*)malloc(128 * sizeof(struct fileEntry)); 

	block_read(0, mounted);
	mounted->signature[8] = '\0';
	
	if(strcmp(mounted->signature, checkSig) != 0){  
		printf("Wrong Signature\n");
		return -1;
	}

	if( block_read(mounted->rootDirectory, root) == -1){
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
	fatBuf = (uint16_t*)malloc(sizeof(uint16_t) * 2048 * mounted->fatBlockAmt);
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
		printf("Index: %d , FAT: %d\n", i, fatBuf[i]);
		
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

/*
int fs_create(const char *filename)
{
	return 0;
}

int fs_delete(const char *filename)
{
	return 0; 
}

int fs_ls(void)
{
	return 0; 
}

int fs_open(const char *filename)
{
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
*/
