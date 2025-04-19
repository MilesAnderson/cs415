#include <stdio.h>
#include <stdlib.h>

int main(){
    char *buf;
    size_t bufsize;
    size_t characters;

    printf(">>> ");
    characters = getline(&buf, &bufsize, stdin);
    printf("%s\n", buf);

}