#include <unistd.h>
#include <termio.h>
#include <stdio.h>



int main(void){
    struct termios raw;
    tcgetattr(STDIN_FILENO,&raw);
    printf("%d",raw.c_cflag);
    return 0;
}