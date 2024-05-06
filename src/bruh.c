#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    struct stat sb;
    printf("%s is%s executable.\n", "bruh.exe", stat("bruh.exe", &sb) == 0 &&
                                                sb.st_mode & S_IXUSR ? 
                                                "" : " not");
    return 0;
}   