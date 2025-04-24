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

        while(whileparam == 0){
            printf(">>> ");

            //Get what was typed
            read = getline(&line, &len, stdin);
            if(read == -1){
                perror(line);
            }
            line[strcspn(line, "\n")] = '\0';
            if(strcmp(line, "exit") == 0){
                whileparam++;
            }
            //-------------------------
            
            command_line scdelim = str_filler(line, ";");
            int i = 0;
            while(scdelim.command_list[i] != NULL){
                command_line spdelim = str_filler(scdelim.command_list[i], " ");
                int j = 0;
                while(spdelim.command_list[j] != NULL){
                    if(spdelim.num_token == 0){
                        //nothing entered
                    }
                    //ls
                    else if(strcmp(spdelim.command_list[j], "ls") == 0){
                        if(spdelim.num_token > 1){
                            printf("Error! Unsupported paramaters for command: ls\n");
                            break;
                        }
                        else{
                            listDir();
                        }
                    }
                    //pwd
                    else if(strcmp(spdelim.command_list[j], "pwd") == 0){
                        if(spdelim.num_token > 1){
                            printf("Error! Unsupported paramaters for command: pwd\n");
                            break;
                        }
                        else{
                            showCurrentDir();
                        }
                    }
                    //mkdir
                    else if(strcmp(spdelim.command_list[j], "mkdir") == 0){
                        if(spdelim.num_token < 2){
                            printf("Error! Missing paramaters for command: mkdir\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            printf("Error! Unsupported paramaters for command: mkdir\n");
                            break;
                        }
                        else{
                            makeDir(spdelim.command_list[1]);
                        }
                    }
                    //cd
                    else if(strcmp(spdelim.command_list[j], "cd") == 0){
                        if(spdelim.num_token < 2){
                            printf("Error! Missing paramaters for command: cd\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            printf("Error! Unsupported paramaters for command: cd\n");
                            break;
                        }
                        else{
                            changeDir(spdelim.command_list[1]);
                        }
                    }
                    //cp
                    else if(strcmp(spdelim.command_list[j], "cp") == 0){
                        if(spdelim.num_token < 3){
                            printf("Error! Missing paramaters for command: cp\n");
                            break;
                        }
                        else if(spdelim.num_token > 3){
                            printf("Error! Unsupported paramaters for command: cp\n");
                            break;
                        }
                        else{
                            copyFile(spdelim.command_list[1], spdelim.command_list[2]);
                        }
                    }
                    //mv
                    else if(strcmp(spdelim.command_list[j], "mv") == 0){
                        if(spdelim.num_token < 3){
                            printf("Error! Missing paramaters for command: mv\n");
                            break;
                        }
                        else if(spdelim.num_token > 3){
                            printf("Error! Unsupported paramaters for command: mv\n");
                            break;
                        }
                        else{
                            moveFile(spdelim.command_list[1], spdelim.command_list[2]);
                        }
                    }
                    //rm
                    else if(strcmp(spdelim.command_list[j], "rm") == 0){
                        if(spdelim.num_token < 2){
                            printf("Error! Missing paramaters for command: rm\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            printf("Error! Unsupported paramaters for command: rm\n");
                            break;
                        }
                        else{
                            deleteFile(spdelim.command_list[1]);
                        }
                    }
                    //cat
                    else if(strcmp(spdelim.command_list[j], "cat") == 0){
                        if(spdelim.num_token < 2){
                            printf("Error! Missing paramaters for command: cat\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            printf("Error! Unsupported paramaters for command: cat\n");
                            break;
                        }
                        else{
                            displayFile(spdelim.command_list[1]);
                        }
                    }
                    j++;
                }
                free_command_line(&spdelim);
                i++;
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
            int i = 0;
            while(scdelim.command_list[i] != NULL){
                command_line spdelim = str_filler(scdelim.command_list[i], " ");
                int j = 0;
                while(spdelim.command_list[j] != NULL){
                    if(spdelim.num_token == 0){
                        //nothing entered
                    }
                    //ls
                    else if(strcmp(spdelim.command_list[j], "ls") == 0){
                        if(spdelim.num_token > 1){
                            fprintf(stdout, "Error! Unsupported paramaters for command: ls\n");
                            break;
                        }
                        else{
                            listDir();
                        }
                    }
                    //pwd
                    else if(strcmp(spdelim.command_list[j], "pwd") == 0){
                        if(spdelim.num_token > 1){
                            fprintf(stdout, "Error! Unsupported paramaters for command: pwd\n");
                            break;
                        }
                        else{
                            showCurrentDir();
                        }
                    }
                    //mkdir
                    else if(strcmp(spdelim.command_list[j], "mkdir") == 0){
                        if(spdelim.num_token < 2){
                            fprintf(stdout, "Error! Missing paramaters for command: mkdir\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            fprintf(stdout, "Error! Unsupported paramaters for command: mkdir\n");
                            break;
                        }
                        else{
                            makeDir(spdelim.command_list[1]);
                        }
                    }
                    //cd
                    else if(strcmp(spdelim.command_list[j], "cd") == 0){
                        if(spdelim.num_token < 2){
                            fprintf(stdout, "Error! Missing paramaters for command: cd\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            fprintf(stdout, "Error! Unsupported paramaters for command: cd\n");
                            break;
                        }
                        else{
                            changeDir(spdelim.command_list[1]);
                        }
                    }
                    //cp
                    else if(strcmp(spdelim.command_list[j], "cp") == 0){
                        if(spdelim.num_token < 3){
                            fprintf(stdout, "Error! Missing paramaters for command: cp\n");
                            break;
                        }
                        else if(spdelim.num_token > 3){
                            fprintf(stdout, "Error! Unsupported paramaters for command: cp\n");
                            break;
                        }
                        else{
                            copyFile(spdelim.command_list[1], spdelim.command_list[2]);
                        }
                    }
                    //mv
                    else if(strcmp(spdelim.command_list[j], "mv") == 0){
                        if(spdelim.num_token < 3){
                            fprintf(stdout, "Error! Missing paramaters for command: mv\n");
                            break;
                        }
                        else if(spdelim.num_token > 3){
                            fprintf(stdout, "Error! Unsupported paramaters for command: mv\n");
                            break;
                        }
                        else{
                            moveFile(spdelim.command_list[1], spdelim.command_list[2]);
                        }
                    }
                    //rm
                    else if(strcmp(spdelim.command_list[j], "rm") == 0){
                        if(spdelim.num_token < 2){
                            fprintf(stdout, "Error! Missing paramaters for command: rm\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            fprintf(stdout, "Error! Unsupported paramaters for command: rm\n");
                            break;
                        }
                        else{
                            deleteFile(spdelim.command_list[1]);
                        }
                    }
                    //cat
                    else if(strcmp(spdelim.command_list[j], "cat") == 0){
                        if(spdelim.num_token < 2){
                            fprintf(stdout, "Error! Missing paramaters for command: cat\n");
                            break;
                        }
                        else if(spdelim.num_token > 2){
                            fprintf(stdout, "Error! Unsupported paramaters for command: cat\n");
                            break;
                        }
                        else{
                            displayFile(spdelim.command_list[1]);
                        }
                    }
                    j++;
                }
                free_command_line(&spdelim);
                i++;
            }
            free_command_line(&scdelim);
        }

        free(line);
        fclose(input);
        fclose(output);
    }
    return 0;
}