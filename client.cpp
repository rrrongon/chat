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

using namespace std;

const unsigned MAXBUFLEN = 512;

int main(int argc, char* argv[]) {
    int sockfd, rv, flag;
    ssize_t n;
    char buf[MAXBUFLEN];

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
	string oneline;

	/* Does this program have problems? Yes.
 * 	 * The client may be blocked forever after the server crashes or closes
 * 	 	 * the connection. FOREVER: until user types a line. Otherwise, the client 
 * 	 	 	 * program is blocked on getline().
 * 	 	 	 	 */ 		 
    while (getline(cin, oneline)) {
	if (oneline == "quit") {
	    close(sockfd);
	    break;
	} else {
	    write(sockfd, oneline.c_str(), oneline.length());

            n = read(sockfd, buf, MAXBUFLEN);

            if (n<= 0) {
		if (n==0){
		    cout << "server closed" << endl;
		}else {
		    cout <<"something went wrong.." << endl;
		}
		close (sockfd);
		exit(0);
	    }

	    buf[n] = '\0';
	    cout << buf << endl;
	}
    }
}

