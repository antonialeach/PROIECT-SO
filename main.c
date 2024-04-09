#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>

void listFiles(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        return;
    }

    printf("Reading files in %s:\n", dirname);

    struct dirent *entity;
    while ((entity = readdir(dir)) != NULL) {
        printf("%s\n", entity->d_name);
        printf("%d\n", entity->d_type);

        if (entity->d_type == 4 && strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
            char path[PATH_MAX]; // Ensure enough space for the path
            snprintf(path, sizeof(path), "%s/%s", dirname, entity->d_name);
            listFiles(path);
        }
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {
    
    if (argc != 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];

    listFiles(path);

    return 0;
}