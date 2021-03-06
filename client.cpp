#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string>
#include <fstream>
#include <bits/stdc++.h>

using namespace std;

const unsigned MAXBUFLEN = 512;
const int LOGIN=1;
const int LOGOUT=2;
const int CHAT=3;
const int NOTHING=4;
const int Q=5;

int get_command(string);
string convertToString(char* , int );
void sig_usr(int signo);

int sck_dsc=-100;

int main(int argc, char* argv[]) {
    int sockfd, rv, flag;
    ssize_t n;
    char buf[MAXBUFLEN];
    fd_set rset, orig_set;

    struct addrinfo hints, *res, *ressave;

    ifstream infile(argv[1]);
    string line;
    getline(infile, line);

    char host_name_ip[100];
    char text[20];
    sscanf(line.c_str(), "%s : %s",text,host_name_ip);
    cout << "Host name: " << host_name_ip <<endl;

    getline(infile, line);
    char port[20];
    sscanf(line.c_str(), "%s : %s",text,port);
    cout << "Port:" << port << endl;


    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host_name_ip, port, &hints, &res)) != 0) {
	cout << "getaddrinfo wrong: " << gai_strerror(rv) << endl;
	exit(1);
    }

    ressave = res;
    flag = 0;
	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) 
			continue;
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
			flag = 1;
			break;
		}
		close(sockfd);
	} while ((res = res->ai_next) != NULL);
	freeaddrinfo(ressave);

	if (flag == 0) {
		fprintf(stderr, "cannot connect\n");
		exit(1);
	}

// add sockfd and getline in select
    sck_dsc = sockfd;
    int maxf = 0;
    FD_ZERO(&orig_set);
    FD_SET(STDIN_FILENO, &orig_set);
    FD_SET(sockfd, &orig_set);
    if (sockfd > STDIN_FILENO) maxf = sockfd+1;
    else maxf = STDIN_FILENO + 1;
    
    if (signal(SIGINT, sig_usr) == SIG_ERR){
        cout << "sigint invoked"<<endl;
    }

 // program is blocked on getline().    
    while (1) {
	rset = orig_set;
	select(maxf, &rset, NULL,NULL,NULL);
	if (FD_ISSET(sockfd, &rset)){
	    if (n=read(sockfd, buf, MAXBUFLEN)==0){
		printf("server shut\n");
                close (sockfd);
		exit(0);
	    }else {
	    	cout <<"->"<< buf << endl;
	    }
	}else if (FD_ISSET(STDIN_FILENO, &rset)){
	    if (fgets(buf, MAXBUFLEN, stdin)== NULL) exit(0);
	    buf[strlen(buf)]='\0';    
	    string oneline(buf);
                int command_no = get_command(oneline);
                switch(command_no){
		    case LOGIN:
		        //string user_name = getUserName(oneline);
		        write(sockfd, oneline.c_str(), oneline.length());
		        break;
		    case LOGOUT:
			write(sockfd, oneline.c_str(), oneline.length());
		        break;
		    case CHAT:
	    		write(sockfd, oneline.c_str(), oneline.length());
		        break;
		    case Q:
			close(sockfd);
			cout << "socket has been closed"<< endl;
			cout << "****************** Thank you for using FUN CHAT***************** " << endl;
			exit(0);
		    default:
		        cout << "please choose login/logout/chat/quit command" <<endl;
		        break;
	        }
	   
	}
    }
}


int get_command(string oneline){
    char command[100]="";
    for (int i=0;i<oneline.length();i++){
	if (isalpha(oneline[i])){
	    command[i]=oneline[i];
        }else {
	    break;
	}
    }

    string cmd(command);
    if (cmd == "login"){
	return LOGIN;
    }
    else if (cmd == "logout") {
	return LOGOUT;
    }
    else if (cmd == "chat"){
	return CHAT;
    }
    else if (cmd == "exit"){
	return Q;
    }
    else{
	return NOTHING;
    }
}

string convertToString(char* a, int size)
{
    int i;
    string s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

void sig_usr(int signo){
    if(signo == SIGINT){
        string l = "logout";
	if (sck_dsc>0){
            write(sck_dsc, l.c_str(),l.length());
            close(sck_dsc);
        }
	cout << "\nSIGINT received. Logging out if logged in and shutting."<< endl;
	exit(1);
    }
}
