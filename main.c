#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>

void parse_directory(const char *dirname, int depth) {
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        printf("Error at opening directory\n");
        return;
    }

    struct dirent *entity;
    while ((entity = readdir(dir)) != NULL) {
        if (strcmp(entity->d_name, ".") == 0 || strcmp(entity->d_name, "..") == 0) {
            continue;
        }

        for (int i = 0; i < depth; ++i) {
            printf("|   ");
        }

        printf("|-- %s", entity->d_name);

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entity->d_name);
        struct stat st;
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                printf(" (Directory)\n");
                parse_directory(path, depth + 1);
            } else if (S_ISREG(st.st_mode)) {
                printf(" (File)\n");
            } else {
                printf(" (Other)\n");
            }
        } else {
            printf(" (Error getting information)\n");
        }
    }

    closedir(dir);
}

int main() {
    parse_directory("/home/antonia/Desktop/dir exemple", 0); 
    return 0;
}

