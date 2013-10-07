#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 5000
#define CLIENT_QUEUE_SIZE 10
#define CLIENT_BOUND 24
#define USERNAME_BOUND 20

const char* REJECT = "REJECT";
const int REJECT_SIZE = sizeof(REJECT);

const char* ACCEPT = "ACCEPT";
const int ACCEPT_SIZE = sizeof(ACCEPT);

void* chat_with_client(void* arg);
int client_count = 0;

// Struct holding all client info
struct client
{
    char username[USERNAME_BOUND];
    int socket;
    int admin;
    int group;
    int client_num;
};

client* clients[CLIENT_BOUND];

int main(int argc, char **argv)
{

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int len;

	struct sockaddr_in serveraddr, clientaddr;
	struct timeval to;

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9876);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	to.tv_sec = 5;
	to.tv_usec = 0;

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	
	listen(sockfd, CLIENT_QUEUE_SIZE);

	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	//setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to));

	len = sizeof(clientaddr);
	client* c1;
	// INFINITE!!!!!
	while(1)
	{
		int status;
		int client_socket = accept(sockfd, (struct sockaddr*) &clientaddr, (socklen_t*) &len);
		pthread_t thread;		
		
		c1 = (client*) malloc(sizeof client);
		c1->socket = client_socket;
		c1->client_num = client_count;

		// Store the client socket for access later and create a new thread
		if(client_count < CLIENT_BOUND)
		{	
			clients[client_count] = c1;
			client_count++;
			printf("num clients: %d\n", client_count);

			printf("Creating a new thread!! Here's its socket: %d\n", client_socket);
			fflush(stdout);

			// Send an accept message to the client which allows them to connect
			send(c1->socket, ACCEPT, ACCEPT_SIZE, 0);

			if((status = pthread_create(&thread, NULL, &chat_with_client, (void*) c1)) != 0)
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

			send(c1->socket, REJECT, REJECT_SIZE, 0);
		}
	}

	free(c1);

  return 0;
}

void* chat_with_client(void* arg)
{
	client* c = (client*) arg;
	char line[SIZE];
	int n;

	//printf("%lu\n", c);
	strcpy(line, "Please enter a username: ");
	send(c->socket, line, strlen(line), 0);
	
	n = recv(c->socket, line, SIZE, 0);
	line[n] = '\0';

	strcpy(c->username, line);
	printf("New username, %s, has joined the chat!\n", c->username);

	while(1)
	{

		n = recv(c->socket, line, SIZE, 0);

		line[n] = '\0';

		if(n < 0)
		{
			printf("Sorry, the client was too slow.\n");
			return (void*)1;
		}

		if(strcmp(line, "/list"))
		{
		    for(int i = 0; i < client_count; i++)
		    {
			printf("%d ", i);
			client* temp = clients[i];
			//	printf("%lu\n", &temp);
			printf("username: %s\n", temp->username);
			strcpy(line, temp->username);
			send(c->socket, line, strlen(line), 0);
		    }
		}
		else if(strcmp(line, "quit"))
		{
		    break;
		}

		printf("Got %d bytes from the client: %s\n", n, line);
		fflush(stdout);
		send(c->socket, line, strlen(line), 0);
	}
	
	client_count--;
	close(c->socket);
}






