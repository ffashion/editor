#include <unistd.h>
#include <termio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#define CTRL_KEY(k) ((k) & 0x1f) 

/*** data ***/
struct  editorConfig{
    struct termios orig_termios;
};
struct editorConfig E;
/*** terminal ***/
void die(const char* s){
    write(STDOUT_FILENO,"\x1b[2J",4);
    write(STDOUT_FILENO,"\x1b[H",3); 
    perror(s);
    exit(-1);
}
void disableRawMode(){
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.orig_termios);
}
void enableRawMode(){
    if(tcgetattr(STDIN_FILENO,&E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG |IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | INPCK| ISTRIP | BRKINT);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1) die("tcsetattr");
    
    
}
//读取一个字符
char editorReadKey(){
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO,&c,1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
       
    }
    printf("%d((%c))\n",c,c);
    return c;
}
void editorProcessKeypress(){
    char c = editorReadKey();
    switch (c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO,"\x1b[2J",4);
            write(STDOUT_FILENO,"\x1b[H",3); 
            exit(0);
            break;
    }
}
/*** output ***/
//写24次~
void editorDrawRows(){
    for(int y=0;y<24;y++){
        write(STDOUT_FILENO,"~\r\n",3);
    }
}
//清屏
void editorRefreshScreen(){
    write(STDOUT_FILENO,"\x1b[2J",4);
    //定位光标在左上角,<ESC>[20,20H 将光标定位在20，20处，默认的位置是1，1，所以位于左上角
    write(STDOUT_FILENO,"\x1b[H",3); 
    editorDrawRows();
    //重新定位光标在11处
    write(STDOUT_FILENO,"\x1b[H",3);
}


int main(void){
    enableRawMode();
    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }


    return 0;
}