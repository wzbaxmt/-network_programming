#include <stdio.h>
#include <stdlib.h> //EXIT_FAILURE
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <arpa/inet.h>//网络地址处理方法,缺少能编译通过，但运行时会出错


#define SIZE sizeof(fd_set)*8
int main(int argc, char **argv)
{
    if (argc == 3)
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            fprintf(stdout, "argv[%d]=%s\t", i, argv[i]);
        }
        fprintf(stdout, "\n");
    }
    //创建套接字
    int socket_fd;
    do
    {
        //socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        fprintf(stdout, "socket creating\n");
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    } while (socket_fd == -1);
    fprintf(stdout, "socket create success ,%d\n", socket_fd);
    //初始化套接字

    struct sockaddr_in sin;
    sin.sin_family = PF_INET;
    if (argc == 3)
    {
        sin.sin_port = htons(atoi(argv[2]));
        sin.sin_addr.s_addr = inet_addr(argv[1]);
        fprintf(stdout, "1 init sockaddr_in success,port=%s,ip=%s\n", argv[2], argv[1]);
    }
    else
    {
        const char *port = "8183";
        const char *ip = "192.168.0.1";
        sin.sin_port = htons(atoi(port));
        sin.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。
        fprintf(stdout, "2 init sockaddr_in success,port=%s,ip=%s\n", port, ip);
    }

    //绑定套接字
    int ret;
    do
    {
        fprintf(stdout, "socket binding\n");
        ret = bind(socket_fd, (struct sockaddr *)&sin, sizeof(sin));
    } while (ret);
    fprintf(stdout, "bind success\n");
    //监听套接字
    do
    {
        fprintf(stdout, "socket listening\n");
        ret = listen(socket_fd, 100);
    } while (ret);
    fprintf(stdout, "listen success\n");
    int fds[SIZE];//定义文件描述符集合的数组，大小为1024；
    const int num = sizeof(fds)/sizeof(fds[0]);
    int i = 0;
    for (i = 0; i < num; i++)
    {
        fds[i] = -1; //-1为无效
    }
    fds[0] = socket_fd;
    while (1)
    {
        int max_fd = -1; //文件描述符值
        fd_set rfds;     //定义读文件描述符集
        FD_ZERO(&rfds);  //对文件描述符集初始化，放在循环内，防止select重复调用
        //遍历数组，知道哪些文件描述符需要添加至文件描述符集
        for (i = 0; i < num; i++)
        {

            if (fds[i] == -1)
            {
                continue;
            }
            FD_SET(fds[i], &rfds);
            if (max_fd < fds[i])
            {
                max_fd = fds[i];
            }
        }
        struct timeval timeout = {0, 0}; //对timeout进行初始化，timeout参数为结构体类型，&timeout就是0；

        //等（select函数）
        switch (select(max_fd + 1, &rfds, NULL, NULL, /*&timeout*/ NULL))
        {
        case 0:
            printf("timeout ...\n");
            break;
        case -1:
            perror("select");
            break;
        default:
        {
            //at least one fd ready!
            //至少有一个文件描述符就绪
            for (i = 0; i < num; i++)
            {
                if (fds[i] == -1)
                {
                    continue;
                }
                //socket_fd 就绪，说明来了一个新连接
                if (i == 0 && FD_ISSET(socket_fd, &rfds))
                {
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int connect_fd = accept(socket_fd,(struct sockaddr *)&client,&len); //调用accept从已连接队列中提取客户连接
                    //accept失败
                    if (connect_fd < 0)
                    {
                        perror("accept");
                        continue;
                    }
                    //获得新连接
                    printf("get a new client:[%s:%d]\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
                    //此时并不能直接读写，accept只能保证连接获得，读写事件是否就绪却并不知道（也就是说下一个连接有没有进来）
                    //重新让select负责，添加至读文件描述符集
                    int j = 1; //第一个已被占用，所以j =1；
                    for (; j < num; j++)
                    {
                        if (fds[j] == -1)
                        {
                            break;
                        }
                    }
                    if (j == num) //rfds已经满了
                    {
                        printf("client is full!\n");
                        close(connect_fd);
                        continue;
                    }
                    else
                    {
                        fds[j] = connect_fd;
                    }
                }
                else if (i != 0 && FD_ISSET(fds[i], &rfds)) //普通套接字，普通事件就绪
                {
                    //normal fd ready
                    char buf[1024];
                    ssize_t s = read(fds[i], buf, sizeof(buf) - 1);
                    if (s > 0) //读成功
                    {
                        buf[s] = 0;
                        printf("client# %s\n", buf);
                    }
                    else if (s == 0) //client关闭
                    {
                        close(fds[i]);
                        fds[i] = -1;
                        printf("client is quit!\n");
                    }
                    else //读失败
                    {
                        perror("read");
                        close(fds[i]);
                        fds[i] = -1;
                    }
                }
                else
                {
                    //文件描述符没有就绪
                }
            }
        }
        break;
        }
    }
}
#if 0
protocol
socket protocol 一般不填,默认值为 0,自动适应
传输层：IPPROTO_TCP、IPPROTO_UDP、IPPROTO_ICMP

SOCK_STREAM SOCK_DGRAM
SOCK_STREAM是基于TCP的，数据传输比较有保障。SOCK_DGRAM是基于UDP的.

AF_INET PF_INET
在windows中AF_INET与PF_INET完全一样. 
而在Unix/Linux系统中，在不同的版本中这两者有微小差别.对于BSD,是AF,对于POSIX是PF.
理论上建立socket时是指定协议，应该用PF_xxxx，设置地址时应该用AF_xxxx。当然AF_INET和PF_INET的值是相同的，混用也不会有太大的问题。

listen的第二个参数
未完成队列的大小
假设未完成队列设置为100，  有并发1000个请求过来。因为未完成队列只有100个。先放100个请求过来处理三次握手。其他的请求直接拒绝。

recv recvfrom
recv的recvfrom是可以替换使用的，只是recvfrom多了两个参数，可以用来接收对端的地址信息，
这个对于udp这种无连接的，可以很方便地进行回复。而换过来如果你在udp当中也使用recv，那么就不知道该回复给谁了，
如果你不需要回复的话，也是可以使用的。另外就是对于tcp是已经知道对端的，就没必要每次接收还多收一个地址，没有意义，要取地址信息，
在accept当中取得就可以加以记录了。

多进程服务器缺点：
1.内存消耗比较大,每个进程都独立加载完整的应用环境
2.cpu消耗偏高,高并发下,进程之间频繁进行上下文切换,需要大量的内存换页操作
3.很低的io并发处理能力,只适合处理短请求,不适合处理长请求

多线程服务器缺点：
1.不方便操作系统的管理
2.VM对内存的管理要求非常高,GC的策略会影响多线程并发能力和系统吞吐量
3.由于存在对共享资源操作,一旦出现线程"死锁"和线程阻塞,很容易使整个应用失去可用性
用他们编写的服务器，当用户增多时，服务器性能也会下降。

select服务器的优点：
1.高性能（与多进程和多线程比较）
（1）select一次等待多个文件描述符；
（2）select的cpu压力低；
（3）等待时间变短，提升了性能；
select服务器的缺点：
（1）每次调用select，都需要把fd集合从用户态拷贝到内核态，这个开销在fd很多时会很大
（2）同时每次调用select都需要在内核遍历传递进来的所有fd，这个开销在fd很多时也很大
（3）select支持的文件描述符数量太小了，默认是1024
#endif