#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "ringbuffer.h"
#include "packet_listener.h"

#define MAX_LINES 128
#define MAX_CMD_SIZE 512
#define min(a, b) ((a<b)? (a):(b))
#define max(a, b) ((a>b)? (a):(b))

/* 1. Ring Buffer->ok
   2. Nodelay()->ok
   3. output_buf改成Ring buffer -> ok
   4. 每次只刷新最新的結果 -> ok
   4. 加入上下鍵滾動封包->ok
   4. 封包資訊轉換成char* msg -> ok
   5. msg放入Ring buffer -> ok
*/

/* 目前功能:
    1. 利用package listener抓到封包
    2. 將封包資訊顯示在output window上
    3. 使用者可以同時輸入指令來控制ui:
        1. stop: 停止抓取封包
        2. exit: 離開程式
        3. top: 滾動output視窗到最上面
        4. down: 滾動output視窗到最下面
        5. 上下方向鍵: 滾動output視窗
*/



// WINDOW *win_output, *win_input;
WINDOW *input_win, *output_win; // output_win is only the border of the output window
WINDOW *input_inner, *output_inner;  // output_subwin is the real window to display output
RingBuffer output_buf;
int input_height = 4;
int line_to_show; // output可顯示的行數
int scroll_offset = 0; // 在buffer中由最新行數向上滾動的偏移量

typedef enum{   // 定義整體輸出狀態
    NO_OUTPUT = 0,   // 停止輸出
    ALL_PACKET, // 顯示所有封包資訊
} output_state;

void add_output_line(const char *msg) {
    /* push msg into output buffer*/
    rb_push_overlapped(&output_buf, msg);
    scroll_offset = 0;
}

void draw_output_line() {
    /* pop an msg from buffer and display on output window */
    line_to_show = LINES - input_height - 2;
    if(output_buf.count == 0) return;
    if(line_to_show > output_buf.count) line_to_show = output_buf.count;

    if(scroll_offset > output_buf.count - line_to_show){
        scroll_offset = output_buf.count - line_to_show;
    }
    if(scroll_offset < 0){
        scroll_offset = 0;
    }

    werase(output_inner);

    pthread_mutex_lock(&output_buf.mutex);
    // 計算開始顯示的index
    int start_idx = (output_buf.tail - scroll_offset - line_to_show + 2*MAX_BUF_SIZE) % MAX_BUF_SIZE;
    // wprintw(output_inner, "start_idx: %d\n", start_idx);
    for(int i=0; i<line_to_show; i++){
        int idx = (start_idx + i) % MAX_BUF_SIZE;
        wprintw(output_inner, "%s\n", output_buf.buffer[idx]);
    }
    // wprintw(output_inner, "%s\n", msg);
    wrefresh(output_inner);

    pthread_mutex_unlock(&output_buf.mutex);
}

void *packet_simulator(void *arg) {
    int count = 0;
    char msg[128];
    while (1) {
        snprintf(msg, sizeof(msg), "Received packet #%d", count++);
        add_output_line(msg);
        sleep(1);
    }
    return NULL;
}

void draw_input(const char *buf, int cursor_pos){
    werase(input_inner);
    wprintw(input_inner, "> %s", buf);
    wmove(input_inner, 0, cursor_pos+2);
    wrefresh(input_inner);
}

int get_cmd(char *cmd_buf){
    // input information
    static int len = 0, cursor_pos = 0;
    static int initialize = 0;

    if(!initialize){
        // this is a new line of command
        memset(cmd_buf, 0, MAX_CMD_SIZE);
        len = cursor_pos = 0;
        initialize = 1;
    }
    
    // non blocking input
    int ch = wgetch(input_inner);
    if(ch == ERR){
        // no input
    }else if(ch == '\n'){
        cmd_buf[len] = '\0';
        initialize = 0;
        return 1;
    }else if(ch == KEY_BACKSPACE || ch == 127){
        // delete last character
        if(cursor_pos > 0){
            memmove(&cmd_buf[cursor_pos-1], &cmd_buf[cursor_pos], len - cursor_pos+1);
            cursor_pos--;
            len--;
        }
    }else if(ch == KEY_LEFT){
        if(cursor_pos > 0) {
            cursor_pos--;
        }
    }else if(ch == KEY_RIGHT){
        if(cursor_pos < len){
            cursor_pos++;
        }
    }else if(ch == KEY_UP){
        // scroll up
        scroll_offset = min(scroll_offset+1, output_buf.count - line_to_show);
    }else if(ch == KEY_DOWN){
        // scroll down
        scroll_offset = max(scroll_offset-1, 0);
    }else if(ch == KEY_HOME){
        // scorll to the top
        scroll_offset = output_buf.count - line_to_show;
    }else if(ch == KEY_DC){
        // scroll to the bottom
        scroll_offset = 0;
    }else if(isprint(ch) && len < MAX_CMD_SIZE - 1){
        // add character to input buffer
        memmove(&cmd_buf[cursor_pos+1], &cmd_buf[cursor_pos], len - cursor_pos+1);
        cmd_buf[cursor_pos] = ch;
        cursor_pos++;
        len++;
    }
    draw_input(cmd_buf, cursor_pos);
    return 0;
}

void tui_loop() {
    char cmd_buf[MAX_CMD_SIZE];
    while (1) {
        // draw an output
        draw_output_line();
        
        if(get_cmd(cmd_buf)){
            /* Output Command */
            // char msg[MAX_MSG_SIZE];
            // sprintf(msg, "Command: %s", cmd_buf);
            // add_output_line(msg);
            /* a line of command is finished */
            if(strcmp(cmd_buf, "exit") == 0){
                stop_packet_listener();
                break;
            }
            if(strcmp(cmd_buf, "stop") ==0){
                stop_packet_listener();
            }
            if(strcmp(cmd_buf, "top") == 0){
                scroll_offset = output_buf.count - line_to_show;
            }
            if(strcmp(cmd_buf, "down") == 0){
                scroll_offset = 0;
            }
        }
        usleep(10000);
    }
}

void init_tui(){
    initscr();
    noecho();
    cbreak();
    int input_width = COLS;
    int output_height = LINES - input_height;
    int output_width = COLS;

    /* Create Output Window */
    output_win = newwin(output_height, output_width, 0, 0);
    box(output_win, 0, 0);
    wrefresh(output_win);
    /* Create Inner Output Window */
    output_inner = derwin(output_win, output_height-2, output_width-2, 1, 1);
    scrollok(output_inner, TRUE);
    /* Create Input Window */
    input_win = newwin(input_height, input_width, output_height, 0);
    scrollok(input_win, TRUE);
    box(input_win, 0, 0);
    wrefresh(input_win);
    /* Create Inner Input window */
    input_inner = derwin(input_win, 2, COLS - 2, 1, 1);
    nodelay(input_inner, TRUE); // set input to non-blocking mode
    keypad(input_inner, TRUE);  // using special key for input
}

void terminate_tui(){
    // delete window
    wclear(output_win);
    wclear(input_win);
    wrefresh(output_win);
    wrefresh(input_win);
    delwin(output_win);
    delwin(input_win);
    endwin();
}

int main() {
    rb_init(&output_buf, OVERLAPPED);

    init_tui();

    pthread_t packet_listener_thread;
    pthread_create(&packet_listener_thread, NULL, start_packet_listener, NULL);
    
    tui_loop();
    
    pthread_join(packet_listener_thread, NULL);
    terminate_tui();
    return 0;
}
