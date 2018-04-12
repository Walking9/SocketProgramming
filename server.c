#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>

#define  SERV_IP "127.0.0.1"
#define  SERV_PORT 6666

void process_conn_server(int s) {
    ssize_t  size = 0;
    char buffer[1024];

    for(;;){
        size = read(s, buffer, 1024);
        if(0 == size) {
            return;
        }

        //构建响应字符, 为接收到客户端字节的数量
        sprintf(buffer, "%d bytes altogether\n", size);
        write(s, buffer, strlen(buffer)+1);
    }
}

int main(void) {
    int ss, sc;                       //socket描述符
    struct sockaddr_in server_addr;   //服务器地址结构
    struct sockaddr_in client_addr;
    int err;                          //返回值
    pid_t pid;                        //分叉的进行id

    ss = socket(AF_INET, SOCK_STREAM, 0);  //建立一个流式套接字
    if(ss < 0) {                      //error
        printf("socket error\n");
        return -1;
    }
    bzero(&server_addr, sizeof(server_addr));  //清零
    server_addr.sin_family = AF_INET;          //协议族
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //本地地址
    server_addr.sin_port = htons(SERV_PORT);   //服务器端口

    //绑定地址结构到套接字描述符
    err = bind(ss, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err < 0) {
        printf("bind error\n");
        return -1;
    }

    //设置监听
    err = listen(ss, 1024);
    if(err < 0) {
        printf("listen error\n");
        return -1;
    }

    for(;;) {
        socklen_t addrlen = sizeof(struct sockaddr);

        sc = accept(ss, (struct sockaddr*)&client_addr, &addrlen);

        if(sc < 0) {
            continue;
        }

        //建立一个新的进程处理到来的连接
        pid = fork();
        if(0 == pid) {
            process_conn_server(sc);
            close(ss);
        } else {
            close(sc);
        }
    }
}