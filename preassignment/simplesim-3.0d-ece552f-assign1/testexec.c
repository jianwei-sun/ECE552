#include <stdio.h>

int  main (int argc, char *argv[])
{
    int i;
    int sum = 0;
  
    if ( argc != 2 ){
        printf("Usage: %s <count>\n", argv[0]);
        exit(5);
    }

    for (i = 1; i <= atoi(argv[1]); i++){
        sum += i; 
    }

    printf("Sum = %d\n", sum);

    return 0;
}
