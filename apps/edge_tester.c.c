#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fs.h>

int main()
{	
	char* s = (char*)malloc(12000 * sizeof(char));
      	s[0] = '\0';
      	for(int i = 0; i < 1000; i++) {
        	strncat(s, "Spongebob SquarePants |", 9);
      	}

      	char* diskname = "disk2.fs";
     	fs_mount(diskname);

      	fs_create("openFile1.a");
      	fs_create("openFile2.c");
      	int a = fs_open("openFile1.a");
      	int c = fs_open("openFile2.c");
      
	fs_info();
	fs_write(a, s, 9000);
	fs_lseek(a, 4090);
	fs_write(c, "012345678910121314151617181920", 15);

      	char* aBuf = (char*)malloc(5000 * sizeof(char));
	char* bBuf = (char*)malloc(5000 * sizeof(char));
	char* cBuf = (char*)malloc(200 * sizeof(char));  
   	
	fs_ls();
	int aLen = fs_read(a, aBuf, 10);
      	printf("Read: %d bytes\n", aLen);
	printf("Buffer: %s\n", aBuf);
	printf("# # # # # # # # # # # # # # # # # # # # # # # # #\n");

	int bLen = fs_read(a, bBuf, 4200);
      	printf("Read: %d bytes\n", bLen);
	printf("Buffer: %s", bBuf);
	printf("# # # # # # # # # # # # # # # # # # # # # # # # #\n");

	fs_lseek(c, 0);
	int cLen = fs_read(c, cBuf, 10);
	printf("Read: %d bytes\n", cLen);
	printf("Buffer: %s\n", cBuf);
	printf("# # # # # # # # # # # # # # # # # # # # # # # # #\n");

	fs_ls();
	fs_close(a);
	fs_close(c);
	fs_delete("openFile1.a");
	fs_ls();
	
      	return 0;
}





