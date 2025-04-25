#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"
#include "command.h"

int main(int argc, char *argv[]){
    //Interactive Shell
    if(argc == 1){
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        int whileparam = 0;

        while(!whileparam){
            printf(">>> ");

            //Get what was typed
            ssize_t read = getline(&line, &len, stdin);
            if(read == -1){
                perror("getline failed");
                break;
            }
            line[strcspn(line, "\n")] = '\0';
            if(strcmp(line, "exit") == 0){
                whileparam++;
                break;
            }
            //-------------------------
            
            command_line scdelim = str_filler(line, ";");
            for(int i = 0; scdelim.command_list[i] != NULL; i++){
                command_line spdelim = str_filler(scdelim.command_list[i], " ");
                int shouldBreak = 0;
                if(spdelim.num_token == 0){
                    //nothing entered
                }
                //ls
                else if(strcmp(spdelim.command_list[0], "ls") == 0){
                    if(spdelim.num_token != 1){
                        printf("Error! Unsupported paramaters for command: ls\n");
                        shouldBreak = 1;
                    }
                    else{
                        listDir();
                    }
                }
                //pwd
                else if(strcmp(spdelim.command_list[0], "pwd") == 0){
                    if(spdelim.num_token != 1){
                        printf("Error! Unsupported paramaters for command: pwd\n");
                        shouldBreak = 1;
                    }
                    else{
                        showCurrentDir();
                    }
                }
                //mkdir
                else if(strcmp(spdelim.command_list[0], "mkdir") == 0){
                    if(spdelim.num_token != 2){
                        printf("Error! Missing paramaters for command: mkdir\n");
                        shouldBreak = 1;
                    }
                    else{
                        makeDir(spdelim.command_list[1]);
                    }
                }
                //cd
                else if(strcmp(spdelim.command_list[0], "cd") == 0){
                    if(spdelim.num_token != 2){
                        printf("Error! Missing paramaters for command: cd\n");
                        shouldBreak = 1;
                    }
                    else{
                        changeDir(spdelim.command_list[1]);
                    }
                }
                //cp
                else if(strcmp(spdelim.command_list[0], "cp") == 0){
                    if(spdelim.num_token != 3){
                        printf("Error! Missing paramaters for command: cp\n");
                        shouldBreak = 1;
                    }
                    else{
                        copyFile(spdelim.command_list[1], spdelim.command_list[2]);
                    }
                }
                //mv
                else if(strcmp(spdelim.command_list[0], "mv") == 0){
                    if(spdelim.num_token != 3){
                        printf("Error! Missing paramaters for command: mv\n");
                        shouldBreak = 1;
                    }
                    else{
                        moveFile(spdelim.command_list[1], spdelim.command_list[2]);
                    }
                }
                //rm
                else if(strcmp(spdelim.command_list[0], "rm") == 0){
                    if(spdelim.num_token != 2){
                        printf("Error! Missing paramaters for command: rm\n");
                        shouldBreak = 1;
                    }
                    else{
                        deleteFile(spdelim.command_list[1]);
                    }
                }
                //cat
                else if(strcmp(spdelim.command_list[0], "cat") == 0){
                    if(spdelim.num_token != 2){
                        printf("Error! Missing paramaters for command: cat\n");
                        shouldBreak = 1;
                    }
                    else{
                        displayFile(spdelim.command_list[1]);
                    }
                }
                else{
                    printf("Error! Unrecognized command:");
                    for(int i = 0; i < spdelim.num_token; i++){
                        printf(" %s", spdelim.command_list[i]);
                    }
                    printf("\n");
                }
                free_command_line(&spdelim);
                if(shouldBreak) break;
            }
            free_command_line(&scdelim);
        }

        free(line);
    }
    //File Mode
    else if(argc > 1){
        //Check paramaters
        if(strcmp(argv[1], "-f") != 0){
            printf("%s is an invalid parameter\n", argv[1]);
            return 1;
        }
        else if(argc < 3){
            printf("Missing input file\n");
            return 1;
        }
        else if(argc > 3){
            printf("Too many parameters\n");
            return 1;
        }
        //--------------------------------

        //Handle files
        FILE* input;
        input = fopen(argv[2], "r");
        if(input == NULL){
            printf("The input file failed to opened.\n");
            return 1;
        }


        FILE* output;
        output = freopen("output.txt", "w", stdout);
        if(output == NULL){
            printf("Something went wrong when creating the output file");
            return 1; 
        }
        //------------------------------

        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while((read = getline(&line, &len, input)) != -1){
            command_line scdelim = str_filler(line, ";");
            for(int i = 0; scdelim.command_list[i] != NULL; i++){
                command_line spdelim = str_filler(scdelim.command_list[i], " ");
                int shouldBreak = 0;
                if(spdelim.num_token == 0){
                    //nothing entered
                }
                //ls
                else if(strcmp(spdelim.command_list[0], "ls") == 0){
                    if(spdelim.num_token != 1){
                        fprintf(stdout, "Error! Unsupported paramaters for command: ls\n");
                        shouldBreak = 1;
                    }
                    else{
                        listDir();
                    }
                }
                //pwd
                else if(strcmp(spdelim.command_list[0], "pwd") == 0){
                    if(spdelim.num_token != 1){
                        fprintf(stdout, "Error! Unsupported paramaters for command: pwd\n");
                        shouldBreak = 1;
                    }
                    else{
                        showCurrentDir();
                    }
                }
                //mkdir
                else if(strcmp(spdelim.command_list[0], "mkdir") == 0){
                    if(spdelim.num_token != 2){
                        fprintf(stdout, "Error! Missing paramaters for command: mkdir\n");
                        shouldBreak = 1;
                    }
                    else{
                        makeDir(spdelim.command_list[1]);
                    }
                }
                //cd
                else if(strcmp(spdelim.command_list[0], "cd") == 0){
                    if(spdelim.num_token != 2){
                        fprintf(stdout, "Error! Missing paramaters for command: cd\n");
                        shouldBreak = 1;
                    }
                    else{
                        changeDir(spdelim.command_list[1]);
                    }
                }
                //cp
                else if(strcmp(spdelim.command_list[0], "cp") == 0){
                    if(spdelim.num_token != 3){
                        fprintf(stdout, "Error! Missing paramaters for command: cp\n");
                        shouldBreak = 1;
                    }
                    else{
                        copyFile(spdelim.command_list[1], spdelim.command_list[2]);
                    }
                }
                //mv
                else if(strcmp(spdelim.command_list[0], "mv") == 0){
                    if(spdelim.num_token != 3){
                        fprintf(stdout, "Error! Missing paramaters for command: mv\n");
                        shouldBreak = 1;
                    }
                    else{
                        moveFile(spdelim.command_list[1], spdelim.command_list[2]);
                    }
                }
                //rm
                else if(strcmp(spdelim.command_list[0], "rm") == 0){
                    if(spdelim.num_token != 2){
                        fprintf(stdout, "Error! Missing paramaters for command: rm\n");
                        shouldBreak = 1;
                    }
                    else{
                        deleteFile(spdelim.command_list[1]);
                    }
                }
                //cat
                else if(strcmp(spdelim.command_list[0], "cat") == 0){
                    if(spdelim.num_token != 2){
                        fprintf(stdout, "Error! Missing paramaters for command: cat\n");
                        shouldBreak = 1;
                    }
                    else{
                        displayFile(spdelim.command_list[1]);
                    }
                }
                else{
                    fprintf(stdout, "Error! Unrecognized command:");
                    for(int i = 0; i < spdelim.num_token; i++){
                        fprintf(stdout, " %s", spdelim.command_list[i]);
                    }
                    fprintf(stdout, "\n");
                }
                free_command_line(&spdelim);
                if(shouldBreak) break;
            }
            free_command_line(&scdelim);
        }

        free(line);
        fclose(input);
        fclose(output);
    }
    return 0;
}