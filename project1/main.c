#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
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

        printf("%s\n", line);
    }

    free(line);
}