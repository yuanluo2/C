#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define INVALID_SOCKET_VALUE (-1)
#define DO_FOREVER for(;;)
#define HTTP_REQUEST_BUF_LEN (4096)
#define HTTP_MSG_OK "HTTP/1.0 200 OK\r\nContent-type: text/html\r\nConnection: close\r\n\r\n<html>OK</html>"

#define PRINT_SPLIT_LINE \
do { \
    printf("=================================================================\n"); \
} while(0)

/* 
    try to parse the given port, 
    if portStr contains no number or too big or too small, 
    then print the error and exit the program. 
*/
static unsigned short parsePort(const char* portStr){
    long port;
    char* endptr;

    port = strtol(portStr, &endptr, 10);

    if ((errno == ERANGE && (port == LONG_MAX || port == LONG_MIN))
                   || (errno != 0 && port == 0)) {
        perror("error strtol()");
        exit(EXIT_FAILURE);
    }

    if (endptr == portStr) {
        fprintf(stderr, "given port is not valid, please give a number between 0 ~ 65535\n");
        exit(EXIT_FAILURE);
    }

    return (unsigned short)port;
}

/*
    create a socket with the given ip and port.
    if any error happens, print the error and exit the program.
*/
static int createHttpSocket(const char* ip, unsigned short port){
    int sock, ret;
    struct sockaddr_in addr_in;

    /* create socket. */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE) {
        perror("error socket()");
        exit(EXIT_FAILURE);
    }

    /* parse ip address. */
    ret = inet_pton(AF_INET, ip, &(addr_in.sin_addr));
    if (ret < 0) {
        perror("error inet_pton()");
        goto tidy_and_clean;
    }
    else if (ret == 0){
        fprintf(stderr, "given ip is not a valid IPv4 dotted-decimal string or a valid IPv6 address string\n");
        goto tidy_and_clean;
    }

    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);

    /* bind ip and port. */
    if (bind(sock, (const struct sockaddr*)(&addr_in), sizeof(struct sockaddr_in)) != 0) {
        perror("error bind()");
        goto tidy_and_clean;
    }

    /* listen. */
    if (listen(sock, SOMAXCONN) != 0){
        perror("error listen()");
        goto tidy_and_clean;
    }

    return sock;

tidy_and_clean:
    close(sock);
    exit(EXIT_FAILURE);
}

/*
    print the current time, format like: 2023-11-11 11:39:39
*/
static void printCurrentTime(void) {
    time_t now;
	time(&now); 
	struct tm* p = localtime(&now);
	printf("[%04d-%02d-%02d %02d:%02d:%02d]\n", 
                    p->tm_year + 1900, 
                    p->tm_mon + 1, 
                    p->tm_mday,
                    p->tm_hour,
                    p->tm_min,
                    p->tm_sec);
    
    fflush(stdout);
}

/*
    print the remote peer's ip and port, format like: 192.168.55.55:8039
*/
static void printRemoteEndpointInfo(int sock){
    struct sockaddr_in addr_in;
    socklen_t len = sizeof(struct sockaddr_in);

    if (getpeername(sock, (struct sockaddr*)(&addr_in), &len) != 0) {
        perror("error getpeername()");
        exit(EXIT_FAILURE);
    }

    char ipBuf[INET6_ADDRSTRLEN];
    if (inet_ntop(addr_in.sin_family, &(addr_in.sin_addr), ipBuf, INET6_ADDRSTRLEN) == NULL){
        perror("error inet_ntop()");
        exit(EXIT_FAILURE);
    }

    printCurrentTime();
    printf("socket connected: %s:%d\n\n", ipBuf, ntohs(addr_in.sin_port));
    fflush(stdout);
}

/* 
    handling http request and send back http response. 
*/
static void handleRequest(int sock){
    char buf[HTTP_REQUEST_BUF_LEN];
    int recvLen;
    int sendLen = strlen(HTTP_MSG_OK);

    recvLen = recv(sock, buf, HTTP_REQUEST_BUF_LEN, 0);
    if (recvLen == 0) {
        fprintf(stderr, "Connection has been closed by remote peer.\n");
    }
    else if (recvLen < 0){
        perror("error recv()");
        exit(EXIT_FAILURE);
    }
    else{
        buf[recvLen] = '\0';
        printf("%s\n", buf);
        PRINT_SPLIT_LINE;
        fflush(stdout);

        send(sock, HTTP_MSG_OK, sendLen, 0);
    }
}

/*
    recv() timed out setting.
*/
static void setRecvTimeout(int sock, time_t sec, time_t u_sec) {
    struct timeval recvTimeOut;
    recvTimeOut.tv_sec = sec;
    recvTimeOut.tv_usec = u_sec;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const void*)&recvTimeOut, sizeof(recvTimeOut));
}

/*
    signal handler, used for SIGINT, just flush the output and exit the program.
*/
static void handleSignalQuit(int signum){
    printf("\nHttp server exit.\n");
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

/*
    signal handler, used for SIGCHLD, to prevent zombie process.
*/
static void handleSignalChild(int signum){
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);

    printCurrentTime();
    printf("\nremove proc id: %d\n", pid);
    PRINT_SPLIT_LINE;
    fflush(stdout);
}

int main(int argc, char* argv[]){
    if (argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return -1;
    }

    /* parse port. */
    unsigned short port = parsePort(argv[1]);

    /* handle Ctrl+C event. */
    struct sigaction actionSIGINT;
    actionSIGINT.sa_handler = handleSignalQuit;
    actionSIGINT.sa_flags = 0;
    sigemptyset(&(actionSIGINT.sa_mask));

    if (sigaction(SIGINT, &actionSIGINT, NULL) < 0){
        perror("error sigaction() for SIGINT");
        exit(EXIT_FAILURE);
    }

    /* handle child process. */
    struct sigaction actionSIGCHLD;
    actionSIGCHLD.sa_handler = handleSignalChild;
    actionSIGCHLD.sa_flags = SA_RESTART;
    sigemptyset(&(actionSIGCHLD.sa_mask));

    if (sigaction(SIGCHLD, &actionSIGCHLD, NULL) < 0){
        perror("error sigaction() for SIGCHLD");
        exit(EXIT_FAILURE);
    }

    /* create http server and accept requests forever. */
    int httpServer = createHttpSocket("0.0.0.0", port);
    int connection;
    pid_t pid;

    printf("\nHttp server started, served on port: %d\n\n", port);
    fflush(stdout);

    DO_FOREVER {
        connection = accept(httpServer, NULL, NULL);
        if (connection == INVALID_SOCKET_VALUE) {
            perror("error accept()");
            continue;
        }

        setRecvTimeout(connection, 5, 0);
        pid = fork();

        if (pid < 0) {
            perror("error fork()");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {        /* child process. */
            close(httpServer);
            printRemoteEndpointInfo(connection);
            handleRequest(connection);

            shutdown(connection, SHUT_RDWR);    /* close the remote connection. */
            close(connection);
            exit(EXIT_SUCCESS);
        } else {                      /* parent process. */
            close(connection);
        }
    }

    shutdown(httpServer, SHUT_RDWR);
    close(httpServer);
}
