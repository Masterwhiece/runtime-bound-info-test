#include <stdlib.h>
#include <stdio.h>

void compareOffsetNSize(int offset, int size)
{
    printf("offset, arraysize: %d, %d\n", offset, size);

    printf("Size is calculated in bytes\n");
    printf("offset: %d\n", offset);
    //size = size * 4;
    printf("array size: %d\n", size);


    if (offset > size)
    {
        printf("\nError! Buffer Overflow입니다...! 프로그램을 종료합니다.\n");
        exit(1);
    }
}