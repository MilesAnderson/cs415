#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include "command.h"

void listDir(){
    printf("you typed ls\n");
}

void showCurrentDir(){
    printf("you typed pwd\n");
}

void makeDir(char *dirName){
    printf("you are creating directory %s\n", dirName);
}

void changeDir(char *dirName){
    printf("you are changing to directory %s\n", dirName);
}

void copyFile(char *sourcePath, char *destinationPath){
    printf("you are copying %s to %s\n", sourcePath, destinationPath);
}

void moveFile(char *sourcePath, char *destinationPath){
    printf("you are moving %s to %s\n", sourcePath, destinationPath);
}

void deleteFile(char *filename){
    printf("you are deleting %s\n", filename);
}

void displayFile(char *filename){
    printf("you are displaying the contents of %s\n", filename);
}