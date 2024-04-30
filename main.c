#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <time.h>
#include <fcntl.h>

#define MAX_ARGS 10

struct metadata {
    char name[NAME_MAX];
    off_t size;
    mode_t permissions;
    time_t last_access;
    time_t last_modification;
};

void get_metadata(const char *path, struct metadata *meta) {
    struct stat st;
        if (stat(path, &st) == 0) {
        strncpy(meta->name, path, NAME_MAX);
        meta->size = st.st_size;
        meta->permissions = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO); 
        meta->last_access = st.st_atime;
        meta->last_modification = st.st_mtime;
    } else {
        perror("Error getting information");
    }
}

void write_metadata(const char *filename, struct metadata *meta) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd != -1) {
        char buffer[4096]; 
        int len = sprintf(buffer, "Name: %s\n", meta->name);
        len += sprintf(buffer + len, "Size: %ld bytes\n", meta->size);
        len += sprintf(buffer + len, "Permissions: %o\n", meta->permissions);
        len += sprintf(buffer + len, "Last access: %s", ctime(&(meta->last_access)));
        len += sprintf(buffer + len, "Last modification: %s", ctime(&(meta->last_modification)));

        write(fd, buffer, len);
        close(fd);
    } else {
        perror("Error opening file for writing");
    }
}

void create_snapshot(const char *dirname, const char *output_dir) {
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

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entity->d_name);

        struct metadata current_meta;
        get_metadata(path, &current_meta);

        char snapshot_filepath[PATH_MAX * 2]; 
        snprintf(snapshot_filepath, sizeof(snapshot_filepath), "%s/%s_snapshot", output_dir, entity->d_name);

        struct metadata old_meta;
        char old_snapshot_filepath[PATH_MAX * 2];
        snprintf(old_snapshot_filepath, sizeof(old_snapshot_filepath), "%s_snapshot", path);
        if (access(old_snapshot_filepath, F_OK) != -1) { 
            get_metadata(old_snapshot_filepath, &old_meta);
            if (memcmp(&current_meta, &old_meta, sizeof(struct metadata)) == 0) {
                continue;
            }
        }

        write_metadata(snapshot_filepath, &current_meta);

        struct stat st;
        if (stat(path, &st) == 0){
            if(S_ISDIR(st.st_mode)) {
                create_snapshot(path, output_dir);
            }
        } else {
            printf("Error getting information from: %s\n", path);
        }
    }

    closedir(dir);
}

void snapshotDirectories(int argc, char **argv){
    if (argc < 4 || argc > MAX_ARGS || strcmp(argv[1], "-o") != 0) {
        perror("./program_exe -o output input1 input2 ...");
        exit(EXIT_FAILURE);
    }

    char *output_path = argv[2];
    mkdir(output_path, S_IRWXU | S_IRWXG | S_IRWXO);
    for (int i = 3; i < argc; i++) {
        char *input_path = argv[i];
        create_snapshot(input_path, output_path);
    }
}

int main(int argc, char **argv) {
  
    snapshotDirectories(argc, argv);

    return 0;
}


