#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#define SERVER_IP "127.0.0.1"
#define PORT 8080



struct tetris;

void tetris_cleanup_io();

void tetris_signal_quit(int);

void tetris_set_ioconfig();

void tetris_init(struct tetris *t,int w,int h);

void tetris_clean(struct tetris *t);

void tetris_print(struct tetris *t);

void tetris_run(int width, int height);

void tetris_new_block(struct tetris *t);

void tetris_new_block(struct tetris *t);

void tetris_print_block(struct tetris *t);

void tetris_rotate(struct tetris *t);

void tetris_gravity(struct tetris *t);

void tetris_fall(struct tetris *t, int l);

void tetris_check_lines(struct tetris *t);

int tetris_level(struct tetris *t);

void check_bomb(struct tetris *t);

void start(int sockfd);


int sock, valread;
struct sockaddr_in serv_addr;
char buffer[1024];
char message[1024];
int BOMB;
int SCORE;
int WIN;

struct tetris_level {
    int score;
    int nsec;
};

struct tetris {
    char **game;
    int w;
    int h;
    int level;
    int gameover;
    int score;
    struct tetris_block {
        char data[5][5];
        int w;
        int h;
    } current;
    int x;
    int y;
};

struct tetris_block blocks[] =
{
    {{"##", 
         "##"},
    2, 2
    },
    {{" X ",
         "XXX"},
    3, 2
    },
    {{"@@@@"},
        4, 1},
    {{"OO",
         "O ",
         "O "},
    2, 3},
    {{"&&",
         " &",
         " &"},
    2, 3},
    {{"ZZ ",
         " ZZ"},
    3, 2}
};

struct tetris_level levels[]=
{
    {0,
        1200000},
    {1500,
        900000},
    {8000,
        700000},
    {20000,
        500000},
    {40000,
        400000},
    {75000,
        300000},
    {100000,
        200000}
};

#define TETRIS_PIECES (sizeof(blocks)/sizeof(struct tetris_block))
#define TETRIS_LEVELS (sizeof(levels)/sizeof(struct tetris_level))

struct termios save;

void
tetris_cleanup_io() {
    tcsetattr(fileno(stdin),TCSANOW,&save);
}

void
tetris_signal_quit(int s) {
    tetris_cleanup_io();
}

void
tetris_set_ioconfig() {
    struct termios custom;
    int fd=fileno(stdin);
    tcgetattr(fd, &save);
    custom=save;
    custom.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(fd,TCSANOW,&custom);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0)|O_NONBLOCK);
}

void
tetris_init(struct tetris *t,int w,int h) {
    int x, y;
    t->level = 1;
    t->score = 0;
    t->gameover = 0;
    t->w = w;
    t->h = h;
    t->game = malloc(sizeof(char *)*w);
    for (x=0; x<w; x++) {
        t->game[x] = malloc(sizeof(char)*h);
        for (y=0; y<h; y++)
            t->game[x][y] = ' ';
    }
}

//刪除所有紀錄
void
tetris_clean(struct tetris *t) {
    int x;
    for (x=0; x<t->w; x++) {
        free(t->game[x]);
    }
    free(t->game);
}

void
tetris_print(struct tetris *t) {
    int x,y;
    for (x=0; x<30; x++)
        printf("\n");
    printf("[LEVEL: %d | SCORE: %d]\n", t->level, t->score);
    for (x=0; x<2*t->w+2; x++)
        printf("~");
    printf("\n");
    for (y=0; y<t->h; y++) {
        printf ("!");
        for (x=0; x<t->w; x++) {
            if (x>=t->x && y>=t->y 
                    && x<(t->x+t->current.w) && y<(t->y+t->current.h) 
                    && t->current.data[y-t->y][x-t->x]!=' ')
                printf("%c ", t->current.data[y-t->y][x-t->x]);
            else
                printf("%c ", t->game[x][y]);
        }
        printf ("!\n");
    }
    for (x=0; x<2*t->w+2; x++)
        printf("~");
    printf("\n");
}

//有沒有碰到底
int
tetris_hittest(struct tetris *t) {
    int x,y,X,Y;
    struct tetris_block b=t->current;
    for (x=0; x<b.w; x++)
        for (y=0; y<b.h; y++) {
            X=t->x+x;
            Y=t->y+y;
            if (X<0 || X>=t->w)
                return 1;
            if (b.data[y][x]!=' ') {
                if ((Y>=t->h) || 
                        (X>=0 && X<t->w && Y>=0 && t->game[X][Y]!=' ')) {
                    return 1;
                }
            }
        }
    return 0;
}

void
tetris_new_block(struct tetris *t) {
    t->current=blocks[random()%TETRIS_PIECES];
    t->x=(t->w/2) - (t->current.w/2);
    t->y=0;
    if (tetris_hittest(t)) {
        t->gameover=1;
    }
}

void
tetris_print_block(struct tetris *t) {
    int x,y,X,Y;
    struct tetris_block b=t->current;
    for (x=0; x<b.w; x++)
        for (y=0; y<b.h; y++) {
            if (b.data[y][x]!=' ')
                t->game[t->x+x][t->y+y]=b.data[y][x];
        }
}

void
tetris_rotate(struct tetris *t) {
    struct tetris_block b=t->current;
    struct tetris_block s=b;
    int x,y;
    b.w=s.h;
    b.h=s.w;
    for (x=0; x<s.w; x++)
        for (y=0; y<s.h; y++) {
            b.data[x][y]=s.data[s.h-y-1][x];
        }
    x=t->x;
    y=t->y;
    t->x-=(b.w-s.w)/2;
    t->y-=(b.h-s.h)/2;
    t->current=b;
    if (tetris_hittest(t)) {
        t->current=s;
        t->x=x;
        t->y=y;
    }
}


//將所有資料向上平移一行，並在最下面那一行新增炸彈，挖空位置隨機
void bomb(struct tetris *t)
{
    for(int w=0; w<12; w++)
    {
        t->game[w][0] = ' ';
        for(int h=1; h<15; h++)
        {
            t->game[w][h-1] = t->game[w][h];
        }
        t->game[w][14] = 'B';
    }

    t->game[rand()%12][14] = ' ';
}

int RecieveBomb()
{   
        valread = read(sock, buffer, 1024);
        for(int i=0; i < 1024; i++)
        {
            if(buffer[i] == 'B')
            {
                //如果遇到B回傳1，代表對方加炸彈
                memset(buffer, 0, sizeof(buffer));
                memset(message, 0, sizeof(message));
                return 1;
            }
            else if(buffer[i] == 'W')
            {
                //收到W代表勝利
                WIN = 1;
                memset(buffer, 0, sizeof(buffer));
                memset(message, 0, sizeof(message));
                return 0;

            }
            else if(buffer[i] == '0')
            {
                //收到0的話提前結束
                memset(buffer, 0, sizeof(buffer));
                memset(message, 0, sizeof(message));
                return 0;
            }

        }
}

void
tetris_gravity(struct tetris *t) {
    int x,y;
    t->y++;
    if (tetris_hittest(t)) {
        t->y--;
        tetris_print_block(t);

        //回傳1代表有加炸彈
        if (RecieveBomb() == 1)
        {
            bomb(t);
            tetris_new_block(t); 
        }
        else
        {
            tetris_new_block(t); 
        }        
        
    }

}

//如果有削掉一行 往下平移一格
void
tetris_fall(struct tetris *t, int l) {
    int x,y;
    for (y=l; y>0; y--) {
        for (x=0; x<t->w; x++)
            t->game[x][y]=t->game[x][y-1];
    }
    for (x=0; x<t->w; x++)
        t->game[x][0]=' ';
}

//是炸彈傳B
void send_bomb1(int sockfd) {
    char message[] = "B";
    size_t len = strlen(message);
    ssize_t bytes_sent = send(sockfd, message, len, 0);
    if (bytes_sent == -1) {
        perror("send");
    }
}
//不是炸彈傳S
void send_bomb2(int sockfd) {
    char message[] = "S";
    size_t len = strlen(message);
    ssize_t bytes_sent = send(sockfd, message, len, 0);
    if (bytes_sent == -1) {
        perror("send");
    }
}

void start(int sockfd)
{
    //傳K給對方表示同時開始
    char message[] = "K";
    size_t len = strlen(message);
    ssize_t bytes_sent = send(sockfd, message, len, 0);
    if (bytes_sent == -1) {
        perror("send");
    }
    printf("wait for the other user...\n");

    while(1)
    {
        int start = 0;
        valread = read(sock, buffer, 1024);
        for(int i=0; i < 1024; i++)
        {
            if(buffer[i] == 'K')
            {
                memset(buffer, 0, sizeof(buffer));
                memset(message, 0, sizeof(message));
                start = 1;
                break;
            }
            else if(buffer[i] == '0')
            {
                memset(buffer, 0, sizeof(buffer));
                memset(message, 0, sizeof(message));
                break;
            }
        }

        if(start == 1)
        {   
            system("clear");
            printf("33333333333\n          3\n          3\n          3\n33333333333\n          3\n          3\n          3\n33333333333\n");
            sleep(1);
            system("clear");

            printf("22222222222\n          2\n          2\n          2\n22222222222\n2\n2\n2\n22222222222\n");
            sleep(1);
            system("clear");
        
            printf("     1\n   1 1\n 1   1\n     1\n     1\n     1\n     1\n     1\n11111111111\n");
            sleep(1);
            system("clear");

            printf("game start!\n");
            sleep(1);
            system("clear");
            break;
        }
    }
    
}

//
//0是false,不是炸彈傳S
//1是true,是炸彈傳B
void
tetris_check_lines(struct tetris *t) {
    int x,y,l;
    int p=100;
    for (y=t->h-1; y>=0; y--) {
        l=1;//判斷加分數
        BOMB=l;//判斷炸彈
        for (x=0; x<t->w && l; x++) {
            if (t->game[x][y]==' ') {
                l=0;//判斷加分數,如果沒有消掉一排,l為false(0)                             
                BOMB=l;//判斷炸彈
            }
        }
        if (l) {
            t->score += p;
            p*=2;
            tetris_fall(t, y);
            y++;
        }
    }
}



int
tetris_level(struct tetris *t) {
    int i;
    for (i=0; i<TETRIS_LEVELS; i++) {
        if (t->score>=levels[i].score) {
            t->level = i+1;
        } else break;
    }
    return levels[t->level-1].nsec;
}

void check_bomb(struct tetris *t)
{
    if(t->score == SCORE)
    {
        send_bomb2(sock);
    }
    else
    {
        send_bomb1(sock);
        int score_diff = 0;
        score_diff = t->score - SCORE;
        if(score_diff != 100)
        {
            score_diff = score_diff / 100;
            while((score_diff / 2) != 1)
            {
                send_bomb1(sock);
                score_diff = score_diff / 2;
            }
        }
        SCORE = t->score;
    }
}

void
tetris_run(int w, int h) {

    struct timespec tm;
    struct tetris t;
    char cmd;
    int count=0;
    tetris_set_ioconfig();
    tetris_init(&t, w, h);
    srand(time(NULL));

    tm.tv_sec=0;
    tm.tv_nsec=1000000;

    tetris_new_block(&t);
    while (!t.gameover) {
        nanosleep(&tm, NULL);
        count++;
        if (count%50 == 0) {
            if(WIN == 0)
            {
                tetris_print(&t);
            }
            else
            {
                break;
            }
            tetris_print(&t);
        }
        if (count%350 == 0) {
            tetris_gravity(&t);
            tetris_check_lines(&t);
            //確認有無炸彈
            check_bomb(&t);
        }
        
        //getchar
        while ((cmd=getchar())>0) {
            switch (cmd) {
                case 'j':
                    t.x--;
                    if (tetris_hittest(&t))
                        t.x++;
                    break;
                case 'l':
                    t.x++;
                    if (tetris_hittest(&t))
                        t.x--;
                    break;
                case 'k':
                    tetris_gravity(&t);
                    break;
                case ' ':
                    tetris_rotate(&t);
                    break;
            }
        }
        tm.tv_nsec=tetris_level(&t);
    }

    if(WIN == 0)
    {
        tetris_print(&t);
        printf("*** YOU ARE LOSER ***\n");
        char message[] = "W";
        size_t len = strlen(message);
        ssize_t bytes_sent = send(sock, message, len, 0);
        sleep(10);
    }
    else
    {
        tetris_print(&t);
        printf("*** YOU ARE WINNER ***\n");
    }


    tetris_clean(&t);
    tetris_cleanup_io();
}
   
int main()

{
    sock = 0;
    valread = 0;
    SCORE = 0;
    WIN = 0;
    for(int i = 0; i < 1024; i++)
    {
        buffer[i] = '0';
        message[i] = '0';
    }
    // 建立socket描述符
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 將IPv4地址從點分十進制轉換為二進制
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 連接到Server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    start(sock);
    tetris_run(12, 15);
    return 0;
}    
   