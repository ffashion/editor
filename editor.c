#include <unistd.h>
#include <termio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#define zr_VERSION "0.0.1"
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
    if(write(STDIN_FILENO,"\x1b[6n",4) != 4) return -1;
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
/***append buffer ***/
//write如果一次只写入一个字节那么终端可能会闪烁
//所以我门定义了此结构体,使得write是一次完成的
struct abuf
{
    char *b;
    int len;
};
//定义一个空的常量buffer
#define ABUF_INIT {NULL,0}
//把字符串s添加到ab里
void abAppend(struct abuf *ab,const char *s,int len){
    char *new = realloc(ab->b,ab->len +len);

    if(new == NULL)return;
    memcpy(&new[ab->len],s,len);
    ab->b = new;
    ab->len += len;
}
//释放空间
void abFree(struct abuf *ab){
    free(ab->b);
}

/*** output ***/
void editorDrawRows(struct abuf *ab){
    for(int y=0;y<E.screenrows;y++){
        if( y== E.screenrows / 3){
            char welcome[80];
            int welcomelen = snprintf(welcome,sizeof(welcome),
            "zr editor --version %s",zr_VERSION);
            if(welcomelen > E.screencols) welcomelen = E.screencols;
            
            //将信息显示到中间
            int padding = (E.screencols - welcomelen) / 2;
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--) abAppend(ab, " ", 1);
            
            abAppend(ab,welcome,welcomelen);
        
        }else{
            abAppend(ab,"~",1);
        }
        
        
        
        //K命令清除当前行的一部分  0为默认,清除光标右边的部分，2清除整行, 1清除光标左边部分
        abAppend(ab,"\x1b[K",3);
        if(y < E.screenrows -1){
            abAppend(ab,"\r\n",2);
        }
    }
}
void editorRefreshScreen(){
    struct abuf ab = ABUF_INIT;
    
    //l命令
    abAppend(&ab,"\x1b[?25l",6);

    abAppend(&ab, "\x1b[H", 3);
    
    editorDrawRows(&ab);
    //h命令
    abAppend(&ab,"\x1b[H",3);
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDIN_FILENO,ab.b,ab.len);
    abFree(&ab);
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