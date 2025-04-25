#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>
#include "command.h"

#define BUF_SIZE 4096
 
void listDir(){
    DIR *dir = opendir(".");
    if(dir == NULL){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "opendir error: ", 15);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        write(STDOUT_FILENO, entry->d_name, strlen(entry->d_name));
        write(STDOUT_FILENO, " ", 1);
    }
    write(STDOUT_FILENO, "\n", 1);
    closedir(dir);
}

void showCurrentDir(){
    char cwd[PATH_MAX];
    
    if(getcwd(cwd, sizeof(cwd)) == NULL ){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "getcwd error: ", 14);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    write(STDOUT_FILENO, cwd, strlen(cwd));
    write(STDOUT_FILENO, "\n", 1);
}

void makeDir(char *dirName){
    if (mkdir(dirName, 0755) == -1) {
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "mkdir error: ", 13);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
    }
}

void changeDir(char *dirName){
    if (chdir(dirName) == -1) {
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "cd error: ", 10);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
    }
}

void copyFile(char *sourcePath, char *destinationPath){
    int sourceFD = open(sourcePath, O_RDONLY);
    if(sourceFD == -1){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "cp error (open src): ", 22);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    struct stat destStat;
    int isDir = (stat(destinationPath, &destStat) == 0 && S_ISDIR(destStat.st_mode));

    char fullDestPath[PATH_MAX];
    if(isDir){
        const char *srcFilename = strrchr(sourcePath, '/');
        srcFilename = srcFilename ? srcFilename + 1 : sourcePath;
        snprintf(fullDestPath, PATH_MAX, "%s/%s", destinationPath, srcFilename);
    }
    else{
        strncpy(fullDestPath, destinationPath, PATH_MAX);
    }

    int destFD = open(fullDestPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(destFD == -1){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "cp error (open dst): ", 22);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
        close(sourceFD);
        return;
    }

    char buffer[BUF_SIZE];
    ssize_t bytesRead;
    while((bytesRead = read(sourceFD, buffer, BUF_SIZE)) > 0){
        write(destFD, buffer, bytesRead);
    }

    if(bytesRead == 1){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "cp error (read): ", 17);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
    }

    close(sourceFD);
    close(destFD);
}

void moveFile(char *sourcePath, char *destinationPath){
    int sourceFD = open(sourcePath, O_RDONLY);
    if(sourceFD == -1){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "mv error (open src): ", 22);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    struct stat destStat;
    int isDir = (stat(destinationPath, &destStat) == 0 && S_ISDIR(destStat.st_mode));

    char fullDestPath[PATH_MAX];
    if(isDir){
        const char *srcFilename = strrchr(sourcePath, '/');
        srcFilename = srcFilename ? srcFilename + 1 : sourcePath;
        snprintf(fullDestPath, PATH_MAX, "%s/%s", destinationPath, srcFilename);
    }
    else{
        strncpy(fullDestPath, destinationPath, PATH_MAX);
    }

    int destFD = open(fullDestPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(destFD == -1){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "mv error (open dst): ", 22);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
        close(sourceFD);
        return;
    }

    char buffer[BUF_SIZE];
    ssize_t bytesRead;
    while((bytesRead = read(sourceFD, buffer, BUF_SIZE)) > 0){
        write(destFD, buffer, bytesRead);
    }

    if(bytesRead == -1){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "mv error (read): ", 17);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
    }

    close(sourceFD);
    close(destFD);

    if(unlink(sourcePath) == 1){
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "mv error (unlink): ", 20);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
    }
}

void deleteFile(char *filename){
    if (unlink(filename) == -1) {
        const char *err = strerror(errno);
        write(STDOUT_FILENO, "rm error: ", 10);
        write(STDOUT_FILENO, err, strlen(err));
        write(STDOUT_FILENO, "\n", 1);
    }
}

void displayFile(char *filename){
    printf("you are displaying the contents of %s\n", filename);
}