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
// meannt to be run with 3 data block disk

int main()
{
    int step = 2;
    char* s = (char*)malloc(10000 * sizeof(char));
    s[0] = '\0';
    for(int i = 0; i < 800; i++) {
      strncat(s, "Spongebob SquarePants |", 9);
      strncat(s, "|", 1);
    }

    if (step == 0) {
      char* diskname = "disk2.fs";
      fs_mount(diskname);
      fs_ls();
      fs_info();
      fs_create("baba.k");
      fs_ls();
      fs_info();

    } else if (step == 1) {
      char* diskname = "disk2.fs";
      fs_mount(diskname);
      fs_ls();
      fs_info();
      int a = fs_open("baba.k");
      int q = fs_write(a, s, 5000);
      printf("Write returned : %d\n", q);
      fs_ls();
      fs_info();

    } else if  (step == 2) {
      char* diskname = "disk2.fs";
      fs_mount(diskname);
      int a = fs_open("baba.k");
      fs_ls();
      fs_info();
      char* b = (char* )malloc(6000* sizeof(char));
      fs_read(a, b, 5000);
      printf("Start: %s\n",b);
      fs_close(a);
      fs_create("offset.q");
      int g = fs_open("offset.q");
      int p = fs_write(g, s, 10);
      printf("Writing with no space left: %d\n", p);
      fs_ls();
      fs_info();
      fs_delete("offset.q");
      fs_ls();
      fs_info();
      fs_delete("baba.k");
      fs_ls();
      fs_info();
      fs_umount();
    }



    return 0;
}
