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
//获取光标位置
int getCursorPosition(int *rows,int *cols){ 
    char buf[32];
    unsigned int i = 0;
    
    //读入缓冲区 一直读取 知道读取到R字符
    if(write(STDIN_FILENO,"\x1b6[",4) != 4) return -1;
    while (i<sizeof(buf) -1){
        if(read(STDIN_FILENO,&buf[i],1) !=1 ) break;
        if(buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    //printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);
    
    // printf("\r\n");
    // char c =0;
    // while (read(STDIN_FILENO,&c,1) == 1){
    //     if(iscntrl(c)){
    //         printf("%d\r\n");
    //     }else{
    //         printf("%d ('%c')\r\n", c, c);
    //     }
    // }
    editorReadKey();
    return -1;
    
}

int getWindowSize(int *rows,int *cols){
    struct winsize ws;
    if(ioctl(STDIN_FILENO,TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0){
        //将光标移动到右下角
        //我们写了2个逃逸字符: C command 将光标移动到右侧, B Command将光标移动到下方 . C和B命令有一点的好处是可以阻止越过屏幕边缘
        //[C Command](http://vt100.net/docs/vt100-ug/chapter3.html#CUF)   [B Command](http://vt100.net/docs/vt100-ug/chapter3.html#CUD)
        //参数说明移动多少步 这里就是999，999的原因时确保到达右边或者下边
        //if(write(STDIN_FILENO,'\x1b[999C\x1b[999B',12) != 12) return -1;
        return getCursorPosition(rows,cols);
    }else{
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}
/*** output ***/
void editorDrawRows(){
    for(int y=0;y<E.screenrows;y++){
        write(STDOUT_FILENO,"~\r\n",3);
    }
}
void editorRefreshScreen(){
    write(STDOUT_FILENO,"\x1b[2J",4);
    
    //定位光标在左上角,比如<ESC>[20,20H 将光标定位在20，20处，默认的位置是1，1，所以位于左上角
    write(STDOUT_FILENO,"\x1b[H",3); 
    
    //写~
    editorDrawRows();
    //重新定位光标在11处
    write(STDOUT_FILENO,"\x1b[H",3);
}

/*** init ***/
void initEditor(){
    //获取窗口大小
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