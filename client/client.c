#include"threadpool.h"
//#include"resource.h"
#include<errno.h>
#include<arpa/inet.h>
#include<unistd.h>
#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 40


void request(void* arg){
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 创建套接字
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // 连接服务器
    while(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))<0 ) {
        if(errno!=ECONNREFUSED){
             perror("Connection failed");
             exit(EXIT_FAILURE);
        }
        printf("Connection refused,try again...\n");
    }
    send(client_socket,arg,sizeof(client_socket),0);
    recv(client_socket,buffer,BUFFER_SIZE,0);
    int result=*((int*)buffer);
    printf("客户%d抢到了%d\n",*((int*)arg),result);
    close(client_socket);
}

/*void test(void* arg){
    int result=0,i=0;
    while(access_resource(i++%100,&result)==-1);

    printf("客户%d抢到了%d\n",*((int*)arg),result);
}*/

int main(){
    ThreadPool threadpool;
    thread_pool_init(&threadpool,10,4,3,CORETHREAD_TIMEOUT_EXIT,50);
    //init_segments();
    int arg[10000]={0};
    int i=0;
    for(i=0;i<10000;i++){
        arg[i]=i;
        thread_pool_add_task(&threadpool,request,(void*)&arg[i]);
    }
    thread_pool_wait_done(&threadpool);
    thread_pool_shutdown(&threadpool);
    //destory_segment();
    return 0;
}