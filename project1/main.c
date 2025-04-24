#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

int main(int argc, char *argv[]){
    //Interactive Shell
    if(argc == 1){
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        int whileparam = 0;

        while(whileparam == 0){
            printf(">>> ");

            read = getline(&line, &len, stdin);
            if(read == -1){
                perror(line);
            }
            line[strcspn(line, "\n")] = '\0';
            if(strcmp(line, "exit") == 0){
                whileparam++;
            }

            command_line test;
            test = str_filler(line, " ");
            int i = 0;
            while(test.command_list[i] != NULL){
                printf("%s\n", test.command_list[i]);
                i++;
            }
            free_command_line(&test);
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

        //If Paramaters are correct
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

        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while((read = getline(&line, &len, input)) != -1){
            command_line test;
            test = str_filler(line, " ");
            int i = 0;
            while(test.command_list[i] != NULL){
                fprintf(stdout, "%s\n", test.command_list[i]);
                i++;
            }
            free_command_line(&test); 
        }

        free(line);
        fclose(input);
        fclose(output);
    }
    return 0;
}