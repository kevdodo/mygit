#include "log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "object_io.h"
#include "ref_io.h"


void mygit_log(const char *ref) {
    (void)ref;
    printf("Not implemented.\n");
    size_t length; 
    read_object(ref, COMMIT, length);
}
