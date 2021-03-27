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

using namespace std;

const unsigned MAXBUFLEN = 512;

int getPort(char * argv[]);

int main(int argc, char* argv[]){
    int serv_sockfd, cli_sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t sock_len;
    pid_t child_pid;
    ssize_t n;
    char buf[MAXBUFLEN];

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
    
    for (; ;){
	sock_len = sizeof(cli_addr);
	cli_sockfd = accept(serv_sockfd,(struct sockaddr *)&cli_addr, &sock_len);
	
        printf("remote machine = %s ", inet_ntoa(cli_addr.sin_addr));

	if ((child_pid == fork())==0){

	    close(serv_sockfd); // close the listen socket in child process
            printf("remote machine = %s ", inet_ntoa(cli_addr.sin_addr));
            
	    while ((n = read(cli_sockfd, buf, MAXBUFLEN)) > 0) {
		buf[n] = '\0';
		cout << buf << endl;
		write(cli_sockfd, buf, strlen(buf));
	    }
            if (n == 0) {
		cout << "client closed" << endl;
	    } else {
		cout << "something wrong" << endl;
	    }
	    close(cli_sockfd);
	    exit(0);
	}
	close(cli_sockfd);
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
