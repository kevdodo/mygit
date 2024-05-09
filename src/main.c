#include <stdio.h>
#include <string.h>
#include <stdlib.h>
char **split_path_into_directories(char *path) {
    // Count the number of directories in the path
    int count = 0;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') {
            count++;
        }
    }

    // Allocate memory for the array of directory names
    char **directories = malloc((count + 2) * sizeof(char *));
    if (directories == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    // Split the path into directory names
    int index = 0;
    char *token = strtok(path, "/");
    while (token != NULL) {
        directories[index] = token;
        token = strtok(NULL, "/");
        index++;
    }
    directories[index] = NULL;  // Null-terminate the array

    return directories;
}

int main() {
    char path[] = "/home/user/documents/file.txt";
    char **directories = split_path_into_directories(path);

    // Print the directory names
    for (int i = 0; directories[i] != NULL; i++) {
        printf("%s\n", directories[i]);
    }

    // Free the array of directory names
    free(directories);

    return 0;
}