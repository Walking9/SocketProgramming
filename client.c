#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERV_IP "119.75.217.109"
#define PORT 443

int main(int argc, char** argv) {
    int s;
    struct sockaddr_in server_addr;  //服务器地址结构

    s = socket(AF_INET, SOCK_STREAM, 0);   //建立一个流式套接字
    if(s < 0) {
        printf("socket error\n");
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));    //清零
    //或者memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;    //协议族
    server_addr.sin_port = htons(PORT);
    //将用户输入的字符串类型的ip地址转为整型
    inet_pton(AF_INET, SERV_IP, &server_addr.sin_addr.s_addr);

    connect(s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
    printf("my port: .., connected! now, closing...\n");
    close(s);
    return 0;
}
