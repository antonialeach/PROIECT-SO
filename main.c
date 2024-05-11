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
#include <sys/wait.h>

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

int check_missing_permissions(const char *filename) {
    // Function that verifies if the file has all the permissions missing
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }

    if ((st.st_mode & S_IRUSR) == 0 && (st.st_mode & S_IWUSR) == 0 && (st.st_mode & S_IXUSR) == 0) {
        return 1;
    }

    return 0;
}

void create_snapshot(const char *dirname, const char *output_dir, const char *isolated_space_dir) {
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

        struct stat st;
        if (stat(path, &st) == 0){
            if(S_ISDIR(st.st_mode)) {
                create_snapshot(path, output_dir, isolated_space_dir);
            }
            else if (S_ISREG(st.st_mode))
            {
                if(check_missing_permissions(path) == 0) // If the file has permissions, we get the necessary data and create the snapshot
                {
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

                }
                else // If the file doesn't have permissions, we isolated it in a safe space
               {
                    char buff[51], *p;
                    int pfd[2], pid, status;

                    if (pipe(pfd) < 0)
                    {
                        exit(1);
                    }

                    pid = fork();

                    if (pid < 0)
                    {
                        exit(1);
                    }
                    if (pid == 0)
                    {
                        close(pfd[0]); 
                        dup2(pfd[1], 1); 
                        execlp("bash", "bash","verify_for_malicious.sh", path, (char*)NULL);
                        exit(1);
                    }

                    close(pfd[1]);
                    read(pfd[0], buff, 50);
                    close(pfd[0]);
                    
                    p = strtok(buff, " \t\n\0");

                    if (strcmp(p, "safe") != 0)
                    {
                        int pid2, status2;
                        pid2 = fork();

                        if (pid2 < 0)
                        {
                            exit(1);
                        }
                        if (pid2 == 0)
                        {
                            execlp("mv", "mv", path, "-t", isolated_space_dir, (char*)NULL);
                            exit(1);
                        }
                        wait(&status2);
                        if (WIFEXITED(status2))
                        {
                            printf("The file %s has been isolated.\n", path);
                        }
                    }

                    wait(&status);
                    if (WIFEXITED(status) && strcmp(p, "safe") == 0)
                    {
                        printf("The file %s is safe.\n", path);
                    }
               }
            }
        } else {
            printf("Error getting information from: %s\n", path);
        }
    }

    closedir(dir);
}


void snapshot_directories(int argc, char **argv){ //Previous function that handles the directories from the command line
    if (argc < 6 || argc > MAX_ARGS || strcmp(argv[1], "-o") != 0) {
        perror("./program_exe -o output -s isolated_space input1 input2 ...");
        exit(EXIT_FAILURE);
    }

    char *output_path = argv[2];
    char *isolated_path = argv[4];
    mkdir(output_path, S_IRWXU | S_IRWXG | S_IRWXO);
    for (int i = 5; i < argc; i++) {
        char *input_path = argv[i];
        create_snapshot(input_path, output_path, isolated_path);
    }
}

void child_process_for_directory(int argc, char **argv)
{
    int status;
    pid_t pid;

    if (argc < 6 || argc > MAX_ARGS || strcmp(argv[1], "-o") != 0){
        perror("./program_exe -o output -s isolated_space input1 input2 ...");
        exit(EXIT_FAILURE);
    }

    char *output_path = argv[2];
    char *isolated_path = argv[4];
    mkdir(output_path, S_IRWXU | S_IRWXG | S_IRWXO);
    mkdir(isolated_path, S_IRWXU | S_IRWXG | S_IRWXO);

    for (int i = 5; i < argc; i++) {
        if ((pid = fork()) < 0) {
            perror("Error forking");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            create_snapshot(argv[i], output_path, isolated_path);
            printf("Snapshot for directory %s created successfully\n", argv[i]);
            exit(EXIT_SUCCESS);
        }
    }

    for (int i = 5; i < argc; i++) {
        pid_t child_pid = wait(&status);
        if (WIFEXITED(status)) {
            printf("Child process with PID %d exited with status %d\n", child_pid, WEXITSTATUS(status));
        }
    }
}

int main(int argc, char **argv) {

    child_process_for_directory(argc, argv);

    return 0;
}


