#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


int is_executable(const char *file_path) {
    return access(file_path, X_OK) == 0;
}
int main(int argc, char **argv)
{
    if (is_executable(".gitlab-ci.yml")) {
    printf("The file is executable.\n");
    } else {
        printf("The file is not executable.\n");
    }
}   