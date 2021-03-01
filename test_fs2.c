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
      char* s = (char*)malloc(10000 * sizeof(char));
      s[0] = '\0';
      for(int i = 0; i < 1000; i++) {
        strncat(s, "Spongebob SquarePants |", 6);
      }

      char* diskname = "disk.fs";
      fs_mount(diskname);
      fs_info();
      fs_create("baba.k");
      printf("created\n");
      fs_ls();
      fs_info();
      int a = fs_open("baba.k");
      fs_lseek(a,0);
      fs_write(a, s, 5000);
      fs_lseek(a, 0);
      char* b = (char* )malloc(5000* sizeof(char));
      fs_read(a, b, 5000);
      printf("Start: %s\n",b);
      fs_close(a);
      printf("deleted\n");
      //fs_delete("baba.k");
      fs_ls();
      fs_info();


      return 0;
}
