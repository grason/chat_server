#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 5000
#define CLIENT_QUEUE_SIZE 10
#define CLIENT_BOUND 24

const char* REJECT = "REJECT";
const int REJECT_SIZE = sizeof(REJECT);

const char* ACCEPT = "ACCEPT";
const int ACCEPT_SIZE = sizeof(ACCEPT);

void* chat_with_client(void* arg);
int clients[CLIENT_BOUND];
int client_count = 0;

int main(int argc, char **argv)
{

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int len;

	struct sockaddr_in serveraddr, clientaddr;
	struct timeval to;

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9874);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	to.tv_sec = 5;
	to.tv_usec = 0;

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	
	listen(sockfd, CLIENT_QUEUE_SIZE);

	int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	//setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to));

	len = sizeof(clientaddr);

	// INFINITE!!!!!
	while(1)
	{
		int status;
		int client_socket = accept(sockfd, (struct sockaddr*) &clientaddr, (socklen_t*) &len);
		pthread_t thread;		
		
		// Store the client socket for access later and create a new thread
		if(client_count < CLIENT_BOUND)
		{	
			clients[client_count++] = client_socket;
			printf("Creating a new thread!! Here's its socket: %d\n", client_socket);
			fflush(stdout);

			// Send an accept message to the client which allows them to connect
			send(client_socket, ACCEPT, ACCEPT_SIZE, 0);

			if((status = pthread_create(&thread, NULL, &chat_with_client, (void*) client_socket)) != 0)
			{
				fprintf(stderr, "Thread error %d: %s\n", status, strerror(status));
				return -1;
			}
		}
		// If there is too many clients connected, print stuff, send a reject message, and continue with the loop
		else
		{
			printf("Maximum number of clients reached!\n");
			fflush(stdout);

			send(client_socket, REJECT, REJECT_SIZE, 0);
		}
	}

  return 0;
}

void* chat_with_client(void *arg)
{
	int client_socket = *((int*)&arg);

	while(1)
	{
		char line[SIZE];
		int n;

		n = recv(client_socket, line, SIZE, 0);

		line[n] = '\0';

		if(n < 0)
		{
			printf("Sorry, the client was too slow.\n");
			return (void*)1;
		}

		printf("Got %d bytes from the client: %s\n", n, line);
		fflush(stdout);
		send(client_socket, line, strlen(line), 0);
	}

	client_count--;
	close(client_socket);
}






