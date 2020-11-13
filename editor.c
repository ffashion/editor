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
    int screenrows;
    int screencols;
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
int getWindowSize(int *row,int *cols){
    struct winsize ws;
    if(ioctl(STDIN_FILENO,TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0){
        return -1;
    }else{
        *row = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

/*** output ***/
//写正确~的次数
void editorDrawRows(){
    for(int y=0;y<E.screenrows;y++){
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

/*** init ***/
void initEditor(){
    if(getWindowSize(&E.screenrows,&E.screencols) == -1) die("getWindowSize");
}
int main(void){
    enableRawMode();
    initEditor();
    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }


    return 0;
}