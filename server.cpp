#include <iostream>
#include <iterator>
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
#include <map>

using namespace std;
pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;

#define MAXCONN 3
const unsigned MAXBUFLEN = 512;
const int LOGIN=1;
const int LOGOUT=2;
const int CHAT=3;
const int NOTHING=4;
const int Q=5;

const int SINGLE_USER = 1;
const int BRD_CAST = 2;

const int  EXISTING_USER = 1;
const int NEW_USER = 2;

int getPort(char * argv[]);
int get_command(string);
string parse_login(string);
int isBroadcast(string msg);
int add_user(string, int);
void write_to_clients(int* sockfds, string buf_msg);
string getTargetUserName (string msg);
string getMsg(string msg, int chat_type);
int* getUserSocketfd(map <string, int>* username_socketfd, string username, int chat_type);
void removeUser(map <string, int>* username_socketfd, string);

map <string, int> username_socketfd;

void *client_handler_thread(void *arg){
    int sockfd;
    ssize_t n;
    char buf[MAXBUFLEN];
    sockfd = *((int *)arg);
    free(arg);

    pthread_detach(pthread_self());
    bool already_login=false;
    string username;
    string target_user;
    int user_type=0;
    int chat_type = -1;

    string l;
    int len;

    while ((n=read(sockfd, buf, MAXBUFLEN))>0){
	buf[n]='\0';
	string client_input(buf);
	int command_type = get_command(client_input);
        
        int *sockfds;
	string chatMsg;

	chat_type=-1;
	switch (command_type){
	    case LOGIN:
		if (!already_login){
		    username = parse_login(client_input);
		    cout << "USER: "<< username<<endl;
		    pthread_mutex_lock(&accept_lock);
		    user_type = add_user(username, sockfd);
		    pthread_mutex_unlock(&accept_lock);	

		    if (user_type == EXISTING_USER){
	 	        cout << "existing user" << endl;
		        string x = "user already exisits";
		        int socks[1] = {sockfd};
		        write_to_clients(socks, x);
		        close(sockfd);
		        cout << "Connection closed"<<endl;
		        return(NULL);
		    }
		    else if (user_type==NEW_USER){
		        cout << "New user" << endl;
		        already_login = true;
		    }
		}

		break;
	    case LOGOUT:
		cout <<"USER: "<< username<< " wants to logout" << endl;
		pthread_mutex_lock(&accept_lock);
		removeUser(&username_socketfd,username);
		pthread_mutex_unlock(&accept_lock);
		l = "you have been logged out";
		len = l.length();
		write(sockfd, l.c_str(),len+1);
		already_login = false;
		break;
	    case CHAT:
		if (already_login){
		    chat_type = isBroadcast(client_input);
		
		        if (chat_type== SINGLE_USER){
			    target_user = getTargetUserName(client_input);
			    cout << username << ": wants to chat with ->" << target_user << endl;
			    sockfds = getUserSocketfd(&username_socketfd, target_user, SINGLE_USER);
			    chatMsg = getMsg(client_input, SINGLE_USER);
			    chatMsg = username+ " >>" + chatMsg;
			    write_to_clients(sockfds,chatMsg);
		        }else{
		    
			    cout <<username << " wants to broadcast message"<<endl;
			    sockfds = getUserSocketfd(&username_socketfd, username , BRD_CAST);
			    chatMsg = getMsg(client_input, BRD_CAST);
			    chatMsg = username + ">>" +chatMsg;
			    write_to_clients(sockfds, chatMsg);
		        }
		}else{
		    l = "please login first to chat";
		    len = l.length();
		    write(sockfd, l.c_str(), len);
		}
		break;
	    default:
		cout << "None of these valid command" << endl;
		chat_type = -1;
		break;
	}	
    }   

    if (n==0){
	cout << "client close" << endl;
        
    }else {
	cout << "something wrong" << endl;
    }

    pthread_mutex_lock(&accept_lock);
    removeUser(&username_socketfd, username);
    pthread_mutex_unlock(&accept_lock);
    
    close(sockfd);
    pthread_exit(NULL);
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
 		
	}

	sock_ptr = (int *) malloc(sizeof(int));
	*sock_ptr = cli_sockfd;

	pthread_create(&tid, NULL, &client_handler_thread, (void *)sock_ptr);
	
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

int get_command(string line){
    char command[100]="";
    for (int i=0;i<line.length();i++){
        if (isalpha(line[i])){
            command[i]=line[i];
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
    else if (cmd == "quit"){
        return Q;
    }
    else{
        return NOTHING;
    }
}

string parse_login(string message){
    int i=0, pos_counter=0;
    string name="";

    for (auto x: message){
	if(x==' ') break;
	else pos_counter++;
    }

    for (i=++pos_counter;i<message.length();i++){
	if (isalpha(message[i]))
	    name = name + message[i];
    }
    return name;
}

int add_user(string username, int sockfd){ 
    
    bool match = false;
    map<std::string, int>::iterator it = username_socketfd.begin();
	
    while(it != username_socketfd.end()){
	if (it->first == username){
	    match = true;
	    break;
	}
	it++;
    }
    if (match){
        return EXISTING_USER;	
    }else{
	username_socketfd[username] = sockfd;
	return NEW_USER;
    }
}

void write_to_clients(int * sockfds, string buf_msg){
    int n = buf_msg.length();
    char buf[n+1];
    strcpy(buf, buf_msg.c_str());
    buf[n] = '\0'; 
    for(int i=0;i<100;i++){
	if (*(sockfds+i)>0){
	    int x = *(sockfds+i);
	    write(x, buf,n+1);
	}
    }
}

int isBroadcast(string msg){
    int counter=0;
    int type = 1;
    int t = -1;
    for (auto x: msg){
	if (x==' '){ counter++;  break;}
	counter++;
    }

    if (msg[counter]=='@'){
	t = SINGLE_USER;
    }else{
	t =  BRD_CAST;
    }

    return t;
}

string getTargetUserName(string msg){
    int counter=0;
    int type = 1;
    string targetUser="";

    for (auto x: msg){
	if (x==' '){ counter++;  break;}
	counter++;
    }

    if (msg[counter]=='@'){
	counter++;
	for (int j=counter;j<msg.length();j++){
	    if(!isalpha(msg[j])) break;
	    else{
		targetUser = targetUser + msg[j];
	    }
	}
	return targetUser;
    }

}

int* getUserSocketfd(map <string, int>* username_socketfd, string username, int chat_type){
    static int sockfds[100];

    for (int i=0;i<100;i++){ sockfds[i]=-1;}
 
    if(chat_type==SINGLE_USER){
	std::map<std::string, int>::iterator it = username_socketfd->begin();
        int counter = 0;

	while (it != username_socketfd->end()){
            if(it->first==username) {
                sockfds[counter] = it->second;
                counter++;
            }
	    it++;
        }
	return sockfds;
    }else{
	std::map<std::string, int>::iterator it = username_socketfd->begin();
	int counter = 0;
	while (it != username_socketfd->end()){
	    if (it->first!=username){
                sockfds[counter]=it->second;
	        counter++;
	    }
	    it++;
        }

	return sockfds;
    }
}

string getMsg(string msg, int chat_type){
    int space_count = 0;
    string chatMsg="";

    if (chat_type==SINGLE_USER){
	space_count = 2;
    }else if (chat_type==BRD_CAST){
	space_count = 1;
    }

    int counter=0;
    for (auto x: msg){
	if (x==' '){
	    space_count--;
	}if (space_count==0){
	    break;
	}
	counter ++;
    }

    for (int j=counter;j<msg.length();j++){
	chatMsg = chatMsg+msg[j];
    }

    return chatMsg;
}

void removeUser(map <string, int>* username_socketfd, string username){
    username_socketfd->erase(username); 
}
