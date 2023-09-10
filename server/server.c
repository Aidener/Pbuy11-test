#include"threadpool.h"
#include <arpa/inet.h>
#include<unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include"resource.h"
#include<stdio.h>
#include<signal.h>

#define MAX_CLIENTS 128
#define PORT 8080
#define BUFFER_SIZE 40

int server_fd, client_fds[MAX_CLIENTS],argc_client_fds[500];           //定义服务器和客户端文件描述符
int client_count=0;
int endflag=1;       //监听结束符号

void handle(void* arg) {
    int clientfd=*((int*)arg);
    int i=0;
    int resource;
    //抢资源
    while(access_resource(i%100,&resource))
        i++;
    char buffer[BUFFER_SIZE];
    recv(clientfd,buffer,sizeof(buffer),0);
   //抢到的资源反馈给客户端
    send(clientfd,&resource,sizeof(resource),0);
    int name=*((int*)buffer);
    //输出提示
    printf("%d抢到了%d\n",name,resource);
    //服务完毕关闭连接
    close(clientfd);
    *(int*)arg=-1;                       //让出空位
}

int getmaxFD(int server_fd,int client_fd[],int len){
    if(len!=0)
         return server_fd>client_fd[len-1]?server_fd:client_fd[len-1];
    else
         return server_fd;
}

void addFD( int client_fd[],int len,int distFD) {
        int i=0;
         while(i<len&&distFD>client_fd[i])
             i++;
        int j=len;
        for(;j>i;j--){
              client_fd[j]=client_fd[j-1];
        }
        client_fd[i]=distFD;
}

void moveFD(int client_fd[],int len,int distpos){
       int i=distpos;
         for(;i<len;i++){
             client_fd[i]=client_fd[i+1];
         }
}

int add_argcFD(int clientfd){
    int i=0;
    while(argc_client_fds[i%500]!=-1)                            //寻找可以存放的空位
    i++;
    argc_client_fds[i%500]=clientfd;
    return i%500;
}

void sigint_handler(int signum) {
    printf("Received Ctrl+C signal (SIGINT).\n");
    endflag=0;
}

int main() {
    
    struct sockaddr_in server_addr, client_addr;  //定义连接信息
    socklen_t client_addr_len;

    //初始化资源
    init_segments();

    //初始化线程池
    ThreadPool threadpool;
    thread_pool_init(&threadpool,10,4,3,CORETHREAD_TIMEOUT_WAIT,500);

    //注册ctrl+c监听处理函数
    signal(SIGINT,sigint_handler);

    // 创建并绑定服务器套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;         //ipv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   //等待分配0000
    server_addr.sin_port = htons(PORT);          //大端转换

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    //监听端口
    if (listen(server_fd, 128) == -1) {         //监听128个连接
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("服务器正在监听端口 %d...\n",PORT);

    fd_set readfds, all_fds;                                 //文件状态掩码
    int i, new_client_fd, ready_clients;             

    // 初始化 client_fds 和 all_fds 集合
    FD_ZERO(&all_fds);
    FD_ZERO(&readfds);

    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    for(i=0;i<500;i++){
        argc_client_fds[i]=-1;                         //传参参数集合
    }

    FD_SET(server_fd, &all_fds);       //添加对服务器监听  
    
    while (endflag) {
        // 将 all_fds 复制给 readfds
        readfds = all_fds;

        // 使用 select 监听多个文件描述符
        ready_clients = select(getmaxFD(server_fd,client_fds,client_count)+1, &readfds, NULL, NULL, NULL);
        if (ready_clients == -1) {
            perror("select");
            break;
        }

        // 检查是否有新的客户端连接
        if (FD_ISSET(server_fd, &readfds)) {
            client_addr_len = sizeof(client_addr);
            new_client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);  //读出新客户端信息

            if (new_client_fd == -1) {
                perror("accept");
            } else {
                // 将新的客户端套接字加入 client_fds 数组
                addFD(client_fds,client_count,new_client_fd);
                client_count++;
                // 将新的客户端套接字加入 all_fds 集合
                FD_SET(new_client_fd, &all_fds);

                printf("新的客户端连接成功。套接字： %d，IP 地址： %s，端口号： %d\n",
                       new_client_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            }
        }

        // 处理其他已准备好读取的文件描述符（客户端连接）
        for (i = 0; i < client_count; i++) {
            if ( FD_ISSET(client_fds[i], &readfds)) {
                  thread_pool_add_task(&threadpool,handle,&argc_client_fds[add_argcFD(client_fds[i])]);
                  FD_CLR(client_fds[i],&all_fds);                                                                  //连接将要关闭，不再监听
                  moveFD(client_fds,client_count,i);
                  client_count--;
            }
        }
    }

    thread_pool_wait_done(&threadpool);
    //关闭线程池
    thread_pool_shutdown(&threadpool);
    // 关闭服务器套接字
    close(server_fd);
    destory_segment();
    return 0;
}
