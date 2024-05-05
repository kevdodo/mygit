#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>

uint16_t get_file_size(uint32_t fsize){
    if (fsize >= 0xfff){
        return 0xfff;
    }
    return (uint16_t) fsize;
}

int main() {

   printf("%04x\n", get_file_size(2));
   printf("Hello, World!");   
   return 0;
}