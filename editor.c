#include <unistd.h>
#include <termio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

struct termios orig_termios;
//关闭Raw模式
void disableRawMode(){
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
}
//开启raw mode
void enableRawMode(){
    #if 0
    // Version 1.0
    struct  termios raw;
    tcgetattr(STDIN_FILENO,&raw);
    
    //Turn off echo
    //c_cflag  == control flags  c_cflag == local flags
    //只要需要修改这些flags 就需要开启raw mode
    raw.c_cflag &= ~(ECHO);
    
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
    #endif
    // Version 1.1
    tcgetattr(STDIN_FILENO,&orig_termios);
    //程序退出时，自动调用函数
    atexit(disableRawMode);
    
    //设置一个新的变量存放数据，在改变全局变量之前
    struct termios raw = orig_termios;
    
    //ICANON 负责关闭canonical mode,这样我们是一个字节一个字节的输入，而不是一行行的输入
    //这样你只要输入q就能退出了 不需要输入enter
    // ISIG 负责关闭Ctrl-C Ctrl-Z信号
    
    //IEXTEN 负责关闭Ctrl-V信号  On some systems, when you type Ctrl-V, the terminal waits for you to type another character and then sends that character literally.
    //
    raw.c_lflag &= ~(ECHO | ICANON | ISIG |IEXTEN);
    
    
    //关闭Ctrl-S 和 Ctrl-Q信号
    //Ctrl-S 使得程序frozen  Ctrl-Q 恢复中止
    
    //ICRNL = Input carriage return New line,吧Ctrl-M读作13 Enter也读作13
    
    // BRKINT(Break Condition) When BRKINT is turned on, a break condition will cause a SIGINT signal to be sent to the program, like pressing Ctrl-C.
    //  ISTRIP causes the 8th bit of each input byte to be stripped, meaning it will set it to 0. This is probably already turned off
    // INPCK enables parity checking, which doesn’t seem to apply to modern terminal emulators.
    raw.c_iflag &= ~(IXON | ICRNL | INPCK| ISTRIP | BRKINT);
    
    //OPOST=OutPut post-processing of output 这样就指挥到新行 不会到行的开始位置，通常output flag只会设置这一项
    //OPOST 会关闭输入处理，不做处理
    raw.c_oflag &= ~(OPOST);
    
    // It sets the character size (CS) to 8 bits per byte
    raw.c_cflag |= (CS8);
    
    //c_cc = C control characters
    //VMIN The VMIN value sets the minimum number of bytes of input needed before read() can return. 
    //We set it to 0 so that read() returns as soon as there is any input to be read
    
    //VTIME  The VTIME value sets the maximum amount of time to wait before read() returns
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
    
}



int main(void){
    //
    enableRawMode();

    #if 0 //version 1.0
    char c;
    //从标准输入读取一个字节到变量c中
    //(1)默认的你的终端处于canonical mode(cooked mode) 有助于你使用bacspace等
    while(read(STDIN_FILENO,&c,1) == 1 && c != 'q'){
        if(iscntrl(c)){
            //所有的Escape Character 以27开始，拥有3个或4个字节
            //Page Up, Page Down, Home, and End
            printf("%d\n",c);
        }
        else
        {
            printf("%d ('%c')\n",c,c);
        }
        
    }
    #endif 
    while(1){
        char c = '\0';
        read(STDIN_FILENO,&c,1);
        if(iscntrl(c)){
            printf("%d\r\n",c);

        }else{
            printf("%d ('%c')\r\n",c,c);
        }
        if(c == 'q') break;
        
    }


    return 0;
}