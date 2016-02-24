#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <ev.h>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <cstring>
#include <thread>
#include <vector>



#include "httpheader.h"
#include "httphandler.h"
#include "common.h"



using namespace std;

//handlers by socket and own for each thread
//thread_local map<int,HttpHandler*> m_serverMap;

const char* dir=NULL;

//for simplicity don't get from OS real number of cores
//static const int numberKernel=2;

//one loop per one work thread
//static struct ev_loop* loops[numberKernel];

//for manage work thread
vector<thread> threads;

struct ev_loop* main_loop=NULL;

std::ofstream out;

//extern void read_data(struct ev_loop *loop, struct ev_io *watcher, int revents);


//static void idle_cb (EV_P_ ev_idle *w, int revents)
//{
//    //TRACE() << "idle!"  << endl;
//}


//void threadFunc(int i)
//{


//    TRACE() << "thread " << std::this_thread::get_id() << "loop = " << i <<  endl;

//    ev_idle idle; // actual processing watcher
//    ev_io io;     // actual event watcher

//    // initialisation
//    ev_idle_init (&idle, idle_cb);
//    ev_idle_start (loops[i],&idle);


//    while(1)
//    {
//        ev_loop(loops[i], 0);
//        TRACE() << "restart loop i =" << i << endl;
//    }

//    TRACE() << "after " << i << endl;
//}


///* Accept client requests */
//void read_data(struct ev_loop *loop, struct ev_io *watcher, int revents)
//{
//    TRACE() << "thread " << std::this_thread::get_id() << endl;

//    if(EV_ERROR & revents)
//    {
//        TRACE() << "got invalid event";
//        return;
//    }

//    if(!(EV_READ & revents))
//    {
//        return;
//    }

//    HttpHandler* handler = NULL;

//    //find handler...
//    if (m_serverMap.find(watcher->fd) != m_serverMap.end())
//        handler = m_serverMap[watcher->fd];
//    else
//    {
//        handler = new HttpHandler(watcher->fd,dir);
//        m_serverMap[watcher->fd] = handler;
//    }

//    //if any error or not keep-alive - close connection and unregister watcher
//    if (handler->processing() <=0)
//    {
//        m_serverMap.erase(watcher->fd);
//        delete handler;

//        shutdown(watcher->fd,SHUT_RDWR);
//        close(watcher->fd);
//        ev_io_stop(loop,watcher);

//    }
//}

void threadFunc(int socket,const char* dir)
{
    HttpHandler handler(socket,dir);

    handler.processing();

    shutdown(socket,SHUT_RDWR);
    close(socket);

}

/* Accept client requests */
void accept_connection(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    TRACE() << "ACCEPT!!!! 1" << endl;
    static int threadForConnect=0;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sd;
    struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));
    TRACE() << "ACCEPT!!!! 2" << endl;


    if(EV_ERROR & revents)
    {
        TRACE() << "got invalid event";
        return;
    }

    // Accept client request
    client_sd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_sd < 0)
    {
        TRACE() << "accept error";
        return;
    }

    TRACE() << "Successfully connected with client.\n";

    //bool needStartThread=false;

    //loop yet doesn't exist
//    if (!loops[threadForConnect])
//    {
//        loops[threadForConnect] = ev_loop_new(EVBACKEND_EPOLL);
//        needStartThread=true;
//    }

//    TRACE() << "ACCEPT!!!! 3 threadForConnect =" << threadForConnect  << endl;

//    // Initialize and start watcher to read client requests
//    ev_io_init(w_client, read_data, client_sd, EV_READ);
//    ev_io_start(loops[threadForConnect], w_client);

//    if (needStartThread)
//    {
//        //create and start thread only after register watcher because without watcher even loop will finished
//        threads.push_back(thread(threadFunc,threadForConnect));
//    }

//    //next incoming request by handling next thread in ring
//    threadForConnect = (threadForConnect + 1) % numberKernel;

    threads.push_back(thread(threadFunc,client_sd,dir));

}

////for gracefully quit
//void final_handler(int signum)
//{
//    TRACE() << "!!!" << endl;
//    int i = 0;

//    while (i < numberKernel)
//    {
//        if (loops[i])
//        {
//            ev_break(loops[i]);
//        }
//        i++;
//    }

//    if (main_loop)
//        ev_break(main_loop);

//    for(thread& thr:threads)
//        thr.join();

//    TRACE() << "bye..." << endl;

//    exit(EXIT_SUCCESS);
//}

void daemonize()
{
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
                 we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void sockPreparing(int& sock,int port,const char* ip)
{

    sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &(serv_addr.sin_addr));
    //serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        TRACE() << "Error " << errno << endl;
        exit(EXIT_FAILURE);
    }

    int on=1;
    //normal rerun..
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        TRACE() << " Error " << errno << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 128))
    {
        TRACE() << "Error " << errno << endl;
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char *argv[])
{
    const char* ip=NULL;
    int port=-1;

    int c=-1;

    while ((c = getopt (argc, argv, "h:p:d:")) != -1)
    {
        switch (c)
        {
        case 'h':
            ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            dir = optarg;
            break;
        default:
            break;
        }
    }

    if (port<0 || !ip || !dir)
    {
        cerr  << "bad params\r\n";
        return -1;
    }

    //without daemon is more convience for debugging
#ifdef __DAEMON__
    daemonize();
    out.open("/tmp/log.txt");
#endif

    //signal(SIGINT, final_handler);
    //signal(SIGTERM, final_handler);

    int sock = 0;

    sockPreparing(sock,port,ip);

    main_loop = ev_default_loop(EVBACKEND_EPOLL);


    ev_io* io = (ev_io*)malloc(sizeof(struct ev_io));
    ev_io_init(io, accept_connection, sock, EV_READ);
    ev_io_start(main_loop, io);

    TRACE() << "main thread" << std::this_thread::get_id() << endl;

    ev_run(main_loop, 0);

    return 0;
}

