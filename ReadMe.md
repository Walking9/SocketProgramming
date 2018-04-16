# nginx_test

nginx服务器压测小工具：

- 多线程编译nginx_test目录下nginx_test.c文件（备注：程序中宏定义URL为 www.baidu.com，可自行修改）
- 支持参数:  -n 连接数、 -t 线程数 、 -c 并发数
- 输出结果：每秒建立连接数
- 原理：多线程编程，每个线程以非阻塞IO模型与服务器建立TCP连接，用epoll函数监听，并计时整个过程