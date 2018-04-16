#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>

//#define SERV_IP "172.18.72.31"
#define URL "www.baidu.com"
#define SERV_PORT 80
#define RUN_TIME 60

int count_;
struct epoll_event ev;//epoll
struct sockaddr_in srv_addr;//地址
pthread_mutex_t mutex;

//记录传递给线程的参数
struct Threads_parameters{
	int concurrency;//并发数
	int connections;//连接数
};

void time_exit(int sig){
	printf("time is over\n");
	exit(0);
}

//开始建立连接
int start_connect(int epfd){
	int sockfd,rtn;

    //    pthread_mutex_lock(&mutex);
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //创建一个socket

	if(sockfd < 0){
		perror("socket error: ");exit(0);
		return -1;
	}
    //    pthread_mutex_unlock(&mutex);

	int opts;                       //设置为非阻塞
	opts = fcntl(sockfd, F_GETFL);
	if(opts < 0) {
		perror("fcntl(F_GETFL)\n");
		exit(1);
	}
	opts = (opts | O_NONBLOCK);
	if(fcntl(sockfd, F_SETFL, opts) < 0) {
		perror("fcntl(F_SETFL)\n");
		exit(1);
	}

	rtn = connect(sockfd, (struct sockaddr *)(&srv_addr), sizeof(srv_addr));
	if(rtn < 0){
		if (errno == EINPROGRESS){//表明连接还在进行
			ev.data.fd = sockfd;
			ev.events = EPOLLIN | EPOLLPRI | EPOLLHUP | EPOLLOUT | EPOLLET | EPOLLERR;
			epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
		} else {                  //如果连接失败,重连
			close(sockfd);
			start_connect(epfd);
			perror("connect error: ");
		}
	} else if(rtn == 0){
		printf("success\n");
	}
	return 0;
}


void wait_epoll(int epfd, struct Threads_parameters tp){
	int count = 0;
	struct epoll_event events[tp.concurrency];
	while(count < tp.connections){
		int nfds = epoll_wait(epfd, events, 20, 500);
		//printf("%d\n",nfds);
		if (nfds > 0){
			for (int i = 0; i < nfds; i++) {//如果连接成功
				if(events[i].events == EPOLLIN || events[i].events == EPOLLPRI
				   || events[i].events == EPOLLHUP || events[i].events == EPOLLOUT){
					//  close(events[i].data.fd);
					//  start_connect(epfd);
					count++;
				}else if(events[i].events == EPOLLERR){//如果连接失败
					perror("error: ");
				}
				close(events[i].data.fd);
				start_connect(epfd);
			}

		}
	}

}

void* start_thread(void* args){
	struct Threads_parameters tp = *((struct Threads_parameters*)args);   //对每个线程进行处理
	int epfd = epoll_create(256);
	for (int i = 0; i < tp.concurrency; i++) {  //创建concurrency个并发
		if(-1 == start_connect(epfd)){          //如果创建socket失败，重新创建
			i--;
		}
	}
	wait_epoll(epfd,tp);  //等这concurrency个并发完成了connections次访问
}

//该程序支持参数: -n 连接数； -c 并发数；  -t 线程数
int main(int argc, char *argv[]) {
	int connect_num = 1;   //总连接数，默认1
	int thread_num = 1;    //线程数，默认1
	int concurrent_num = 1; //并发数，默认1

	for(int i=1; i < argc; i+=2) {
		if(0 == strcmp("-n", argv[i])) {
			if(i+1 < argc)    //越界检测
				connect_num = atoi(argv[i+1]);
			else;
		}
		else if(0 == strcmp("-c", argv[i])) {
			if(i+1 < argc)
				concurrent_num = atoi(argv[i+1]);
			else;
		}
		else if(0 == strcmp("-t", argv[i])) {
			if(i+1 < argc)
				thread_num = atoi(argv[i+1]);
			else;
		}
		else {
			printf("format error!\n");
			exit(1);
		}
	}

	signal(SIGALRM, time_exit);   //设置程序超时时间
	alarm(RUN_TIME);

	struct in_addr **addr_list;
	struct hostent *ht;
	ht = gethostbyname(URL);    //将域名转换为ip地址
	if(ht == NULL){
		perror("url is not exit: ");
		exit(1);
	}
	//访问地址填充
	addr_list = (struct in_addr **)ht->h_addr_list;
	bzero(&srv_addr, sizeof(srv_addr)); //srv_addr填充0
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERV_PORT);
	srv_addr.sin_addr = *addr_list[0]; //Web的IP地址

	/*记录每个线程应该的并发数和总连接数*/
	struct Threads_parameters tp[thread_num];
	int connect_int = connect_num/thread_num;
	int connect_rem = connect_num%thread_num;
	int concurrent_int = concurrent_num/thread_num;
	int concurrent_rem = concurrent_num%thread_num;
	for (int i  = 0; i < thread_num ; i++) {
		if (i<connect_rem){
			tp[i].connections = connect_int+1;
		} else{
			tp[i].connections = connect_int;
		}
		if (i<concurrent_rem){
			tp[i].concurrency = concurrent_int+1;
		} else{
			tp[i].concurrency = concurrent_int;
		}
	}

	struct timeval start_time;
	struct timeval end_time;
	gettimeofday(&start_time, NULL);   //正式运行前后调用该函数，实现计时
	pthread_t tid[thread_num];

	int err;
	pthread_t ntid;
	for (int i = 0; i < thread_num; i++) {
		//创建线程，执行函数start_thread, 传入参数为(void*)&(tp[i]);
		//由于pthread_create函数传参的要求，这里需要做一下void* 指针的转换
		//返回值为0则创建线程成功
		if((err = pthread_create(&ntid, NULL, start_thread, (void*)&(tp[i])))) {
			printf("catn't create thread: %s\n", strerror(err));
			return 1;
		}
		tid[i] = ntid;
	}

	for (int i = 0; i < thread_num; i++) { //等待所有线程结束
		pthread_join(tid[i],NULL);
	}

	gettimeofday(&end_time,NULL);

	long time1 = start_time.tv_sec*1000 + start_time.tv_usec/1000;
	long time2 = end_time.tv_sec*1000 + end_time.tv_usec/1000;
	printf("HTTP每秒连接数：%ld\n",connect_num*1000/(time2-time1));
	return 0;
}
