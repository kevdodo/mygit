#include <stdlib.h>
#include <string.h>

void iterate_directories(char *file_path) {
    char *file_path_copy = strdup(file_path);
    char *curr_directory = malloc(1);  
    *curr_directory = '\0';  
    char *part = strtok(file_path_copy, "/");
    while (part != NULL) {
        curr_directory = realloc(curr_directory, strlen(curr_directory) + strlen(part) + 2);
        strcat(curr_directory, part);  
        strcat(curr_directory, "/");
        printf("Directory: %s\n", curr_directory);
        part = strtok(NULL, "/");
    }
    free(file_path_copy);
    free(curr_directory);
}

int main(){ 
    iterate_directories("asdf/va/vdads/vsad/vcx/asdf.c");
    return 0;
}