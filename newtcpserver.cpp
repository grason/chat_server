#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <utility>
#include <vector>

#define USERNAME_BOUND 20

using namespace std;

struct client 
{
	char username[USERNAME_BOUND];
	int admin;
	int group;
};

int main(int argc, char **argv)
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	char line[5000];
	int len;
	vector<pair<client*, int> > clients; 
	
	//Create an empty set of sockets
	fd_set sockets;
	FD_ZERO(&sockets);

	struct sockaddr_in serveraddr, clientaddr;
	  
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(9875);
	serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	listen(sockfd,10);
	FD_SET(sockfd,&sockets);

	int option_val = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(option_val));
	
	len=sizeof(clientaddr);
	while(1)
	{
		fd_set temp_set = sockets;
		//size,read,write,exceptions,timeout
		int numready = select(FD_SETSIZE,&temp_set,NULL,NULL,NULL);
		int i;
		for(i = 0; i < FD_SETSIZE; i++)
		{
			// If i is in the set...
			if(FD_ISSET(i, &temp_set))
			{
				// If i just connected...
				if(i == sockfd)
				{
					printf("A client connected!\n");
					int clientsocket = accept(sockfd,
								  (struct sockaddr*)&clientaddr,(socklen_t*)&len);
	
					// Send are username request and recieve that username
					strcpy(line, "Please enter a username: ");
					printf("%s\n", line);
					send(clientsocket, line, strlen(line), 0);
					int n = recv(clientsocket, line, 5000, 0);
					
					line[n] = '\0';
					client* c1 = new client();
					strcpy(c1->username, line);
					printf("%s\n", c1->username);
					//Add the client/socket pair to our list of clients
					clients.push_back(make_pair(c1, clientsocket));
					
					//Add the socket to the set of sockets
					FD_SET(clientsocket, &sockets);
				} 
				// Else, we just need to handle input from i
				else 
				{
					int n;
					n = recv(i, line, 5000,0);
					line[n]='\0';
					printf("Got %d bytes from client: %s\n",n,line);
					send(i,line,strlen(line),0);
				}	
			}	
		}
	}

	return 0;
}






