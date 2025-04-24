#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE

int count_token (char* buf, const char* delim){
    if (buf == NULL){
        return 0;
    }

    int count = 0;
    char *token;
    char *ptr;
    char* copy_buffer = (char *)malloc(sizeof(char) * strlen(buf) + 1);
    strcpy(copy_buffer, buf);
    copy_buffer[strlen(buf)] = '\0';
    token = strtok_r(copy_buffer, delim, &ptr);

    while(token != NULL){
        count++;
        token = strtok_r(NULL, delim, &ptr);
    }

    free(copy_buffer);
    return count;
}

command_line str_filler(char* buf, const char* delim){
    command_line ret;

    int j = 0;
    while(buf[j] != '\0'){
        if(buf[j] == '\n'){
            buf[j] = '\0';
            break;
        }
        j++;
    }
    ret.num_token = count_token(buf, delim);

    ret.command_list = (char**)malloc(sizeof(char*) * ret.num_token+1);

    char* ptr;
    char* token = strtok_r(buf, delim, &ptr);
    int i = 0;
    while(token != NULL){
        ret.command_list[i] = (char*)malloc(sizeof(char) * strlen(token)+1);
        strcpy(ret.command_list[i], token);
        i++;
        token = strtok_r(NULL, delim, &ptr);
    }
    ret.command_list[ret.num_token] = NULL;
    return ret;
}

void free_command_line(command_line* command){
    for(int i = 0; i < command->num_token; i++){
        free(command->command_list[i]);
    }
    free(command->command_list);
}