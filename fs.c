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
struct fileEntry* root;
int open = 0;

int fs_mount(const char* diskname)
{
  printf("here1\n");
  char checkSig[9] = "ECS150FS\0";
  printf("here5\n");
  if (block_disk_open(diskname) == -1){
    printf("Invalid open\n");
    return -1;
  }
  printf("here3\n");
  mounted = (struct superBlock*)malloc(sizeof(struct superBlock));
  printf("here4\n");
  root = (struct fileEntry*)malloc(128*sizeof(struct fileEntry));
  printf("here2\n");

  block_read(0, mounted);
  mounted->signature[8] = '\0';

  for(int i = 0; i < 8; i++)
  {
    printf("%c", mounted->signature[i]);
  }
  printf("%c\n", mounted->signature[8]);

  if(strcmp(mounted->signature, checkSig) != 0){
   printf("Wrong Signature\n");
   return -1;
  }

  if( block_read(mounted->rootDirectory, root) == -1){
    printf("Root Directory Error");
    return -1;
  }

  open = 1;
  return 0;

}


int fs_umount(void)
{
  free(mounted);
  free(root);
  open = 0;
	return 0;
}

int fs_info(void)
{
  // find the rdir_free_ratio
  int rdirFree = FS_FILE_MAX_COUNT;
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if ((int)root[i].fileName[0] != 0) {
      rdirFree--;
    }
  }
  printf("here\n");

  // find the fat_free_ratio
  int fatFree = mounted->dataBlockAmt;
  int count = 0;
  uint16_t * fatBuf = (uint16_t* )malloc(2048* sizeof(uint16_t));
  for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/2048); i++) {
    block_read(1 + i, fatBuf);
    for (int j = 0; j < 2048; j++) {
      count++;
      printf("%d\n",fatBuf[j]);
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
  printf("fat_blk_count%d\n", mounted->fatBlockAmt);
  printf("rdir_blk=%d\n", mounted->rootDirectory);
  printf("data_blk=%d\n", mounted->dataBlock);
  printf("data_blk_count=%d\n", mounted->dataBlockAmt);
  printf("fat_free_ratio=%d/%d\n",fatFree, mounted->dataBlockAmt);
  printf("rdir_free_ratio=%d/128\n", rdirFree);
  return 0;
}

int fs_create(const char *filename)
{
  int rootIndex = -1;
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if ((int)root[i].fileName[0] == 0) {
      rootIndex = i;
      root[i].fileName = filename;
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

  return 0;
}

int fs_delete(const char *filename)
{

  int rootIndex = -1;
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if ((int)root[i].fileName == filename) {
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
        if (fatBuf[j] != 65536) {
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



  return 0;
}
/*
int fs_ls(void)
{
	 TODO: Phase 2
  return -1;
}

int fs_open(const char *filename)
{
	 TODO: Phase 3
  return -1;
}

int fs_close(int fd)
{
	 TODO: Phase 3
  return -1;
}

int fs_stat(int fd)
{
	 TODO: Phase 3
  return -1;
}

int fs_lseek(int fd, size_t offset)
{
	 TODO: Phase 3
  return -1;
}

int fs_write(int fd, void *buf, size_t count)
{
	 TODO: Phase 4
  return -1;
}

int fs_read(int fd, void *buf, size_t count)
{
	TODO: Phase 4
  return -1;
}
*/
