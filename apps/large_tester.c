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
        strncat(s, "|", 1);
        if (i == 712) {
          strncat(s, "HGH", 1);
        }
      }

      char* diskname = "disk2.fs";
      fs_mount(diskname);
      fs_info();
      fs_create("baba.k");
      fs_create("offsetFile.q");
      printf("created\n");
      fs_ls();
      int a = fs_open("baba.k");
      int o = fs_open("offsetFile.q");
      fs_write(o, "we all live in a yellw sumbarine", 20);
      fs_lseek(o, 8);
      fs_write(o, "INSERTION", 9);
      char* t = (char* )malloc(5000* sizeof(char));
      fs_read(o, t, 3);
      printf("Yellow Sub: %s\n", t);
      fs_lseek(o, 0);
      fs_read(o, t, 20);
      printf("Yellow Sub: %s\n", t);
      fs_lseek(a,0);
      fs_write(a, s, 5000);
      fs_lseek(a, 0);
      char* b = (char* )malloc(6000* sizeof(char));
      printf("5001: %d, 5002: %d\n", (int)b[5000], (int)b[5001]);
      fs_read(a, b, 5000);
      printf("Start: %s\n",b);
      printf("5001: %d, 5002: %d\n", (int)b[5000], (int)b[5001]);
      free(b);
      fs_close(a);
      fs_close(o);
      fs_info();
      fs_ls();
      printf("deleted\n");
      fs_delete("offsetFile.q");
      fs_ls();
      fs_info();


      return 0;
}
