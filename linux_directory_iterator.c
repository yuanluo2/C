#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define PATH_MAX_LEN 256

void traverseDir(const char* path) {
    DIR* dir;
    struct dirent* entry;
    struct stat fileStat;
    char fullPath[PATH_MAX_LEN];

    dir = opendir(path);
    if (dir == NULL){
        perror("error opendir()");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullPath, PATH_MAX_LEN, "%s/%s", path, entry->d_name);
        printf("%s\n", fullPath);

        if (stat(fullPath, &fileStat) != 0){
            perror("error stat()");
            continue;
        }

        if (S_ISDIR(fileStat.st_mode)){
            traverseDir(fullPath);
        }
    }

    closedir(dir);
}

int main() {
    const char* path = "/home/kdgc";
    traverseDir(path);

    return 0;
}
