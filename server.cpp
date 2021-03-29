#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>


using namespace std;

#define MAXCONN 3
const unsigned MAXBUFLEN = 512;

int getPort(char * argv[]);


void *client_handler_thread(void *arg){
    int sockfd;
    ssize_t n;
    char buf[MAXBUFLEN];
    sockfd = *((int *)arg);
    free(arg);

    pthread_detach(pthread_self());

    while ((n=read(sockfd, buf, MAXBUFLEN))>0){
	buf[n]='\0';
	cout << "client wrote: " << buf << endl;
    }   

    if (n==0){
	cout << "client close" << endl;
    }else {
	cout << "something wrong" << endl;
    }

    close(sockfd);
    return (NULL);
}

int main(int argc, char* argv[]){
    int serv_sockfd, cli_sockfd,c, *sock_ptr;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t sock_len;
    pid_t child_pid;
    ssize_t n;
    char buf[MAXBUFLEN];
    int clients[MAXCONN];

    fd_set allset, rset;
    pthread_t tid;

    int port = getPort(argv);
    cout << "port = " << port << endl;

    // server socket descr4iptor
    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero((void*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    listen(serv_sockfd, 5);

    fprintf(stderr, "Server up and running...");
    
    // Get my local ip address
    struct sockaddr_in name;
    int lenx = sizeof(name);
    if (getsockname(serv_sockfd, (struct sockaddr *)&name, (socklen_t *)&lenx)<0){
	perror("can't get the name");
    }
    char buffer[80];
    const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);
    if ( p!= NULL){
        cout << "IIIIPPPP: " << buffer << endl;
    }
    fprintf(stderr, "Local IP: %s", inet_ntoa(serv_addr.sin_addr));


    fprintf(stderr, "ip = %s , port = %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

    // get domain name
    struct addrinfo  *res, *result;
    int errcode;
    char addrstr[100];
    void *ptr;


    //clear clients array buffer
    for (c=0;c < MAXCONN; c++)
	clients[c]= -1;
    
    FD_ZERO(&allset);
    FD_SET(serv_sockfd, &allset);
    int maxfd = serv_sockfd;
    
    while(1){
	rset = allset;
	select(maxfd+1, &rset, NULL,NULL,NULL);
	if (FD_ISSET(serv_sockfd, &rset)){
	    /* if someone tries to connect */
	    sock_len = sizeof(cli_addr);
	    if ((cli_sockfd = accept(serv_sockfd,(struct sockaddr *)&cli_addr, &sock_len))<0) {
		if (errno == EINTR)
		    continue;
		else {
		    perror(":accept error");
		    exit(1);
		}
	    }
	    
            printf("remote machine = %s \n", inet_ntoa(cli_addr.sin_addr));
	    printf("remote machine port = %d\n", ntohs(cli_addr.sin_port));
 
	    for (c=0; c< MAXCONN; c++){
		if (clients[c]<0){
		    clients[c]=cli_sockfd;
		    FD_SET(clients[c], &allset);
		    break;
		}
	    }

	    if (c == MAXCONN){
		printf("too many connections.\n");
		close(cli_sockfd);
	    }

	    if (cli_sockfd > maxfd) maxfd = cli_sockfd;
	}

	sock_ptr = (int *) malloc(sizeof(int));
	*sock_ptr = cli_sockfd;

	pthread_create(&tid, NULL, &client_handler_thread, (void *)sock_ptr);
	
	/*for (c=0; c<MAXCONN; c++){
	    if (clients[c]<0) continue;
	    if (FD_ISSET(clients[c], &rset)){
		n = read(clients[c], buf, 100);
		if (n==0){
		    close(clients[c]);
		    FD_CLR(clients[c], &allset);
		    clients[c] = -1;
		}else {
		    cout << "received from client: "<< c << "\n msg: " << buf<<endl;
		    write(clients[c], buf, n);
		}
	    }
	}*/
    }
}

int getPort(char * argv[])
{
    string line;
    ifstream infile(argv[1]);   
    getline(infile, line);
    cout << "Line:"<<line<<endl;
 
    int port;
    char text[20];
    sscanf(line.c_str(), "%s : %d",text,&port);
    
    if (port == 0){
	port = 25101;	
    }
    printf("Port %d\n", port);
    return port;
}
