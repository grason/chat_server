#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define SIZE 5000

int main(int argc, char **argv)
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	int n;
	char line[SIZE];
	char line2[SIZE];
	struct sockaddr_in serveraddr;

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9874);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int return_val = connect(sockfd, (struct sockaddr*) &serveraddr,
			sizeof (struct sockaddr_in));
	fflush(stdout);

	n = recv(sockfd, line2, SIZE, 0);
	line2[n] = '\0';

	if(return_val < 0)
	{
		printf("There was a problem connecting to the server!\n");
		return -1;
	}
	else if (strcmp(line2, "REJECT") == 0)
	{
		printf("Maximum clients connected to server. Client shutting down.\n");
		return 0;
	}

	while(1)
	{
		printf("Enter a line: ");
		fgets(line,SIZE,stdin);
		send(sockfd, line, strlen(line), 0);

		int n = recv(sockfd, line2, SIZE, 0);
		line2[n] = '\0';
		printf("Got from server: %s\n", line2);
		fflush(stdout);
	}

	return 0;
}




