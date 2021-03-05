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
// meant to be run with 2 data block disk

int main()
{
    char* s = (char*)malloc(10000 * sizeof(char));
    s[0] = '\0';
    for(int i = 0; i < 1000; i++) {
      strncat(s, "Spongebob SquarePants |", 6);
      strncat(s, "|", 1);
      if (i == 712) {
        strncat(s, "W", 1);
      }
    }

    char* diskname = "disk3.fs";
    fs_mount(diskname);
    fs_info();
    fs_create("baba.k");
    printf("created\n");
    fs_ls();
    int a = fs_open("baba.k");
    int q = fs_write(a, s, 5000);
    printf("Write returned : %d\n", q);
    fs_lseek(a, 0);
    char* b = (char* )malloc(6000* sizeof(char));
    fs_read(a, b, 5000);
    printf("Start: %s\n",b);
    fs_close(a);
    fs_create("offset.q");
    //int g = fs_open("offset.q");
    //fs_close(g);
    printf("here444\n");
    int p = fs_write(2, s, 10);
    printf("Writing with no space left: %d\n", p);
    fs_info();
    fs_ls();
    printf("deleted\n");
    printf("KK: %d\n", fs_delete("offset.q"));
    fs_ls();
    fs_info();


    return 0;
}
