#include <stdio.h>
#include <stdlib.h>

void exitFunc(char *str){
    printf("%s는 free된 변수입니다.\n", str);
    exit(1);
}