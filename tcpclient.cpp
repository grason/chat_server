#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <termios.h> 
#include <vector>
#include <string>
#include <sstream>

using namespace std;  

/* splits the string into tokens */
// This function is not needed because of the strtok function. See code below
vector<string> &split(const string &s, char delim, vector<string> &elems) {
	stringstream ss(s); 
	string item; 
	while (std::getline(ss, item, delim)) {
		elems.push_back(item); 
	}
	return elems; 
} 

void mygetline(char *line, int len) {
	char c; 
	int numchar = 0; 
	bzero(line, len); 
	while((c=getchar()) != '\n') {
		if(c != 8 && c != 127)
		{
			line[numchar]=c; 
			numchar++; 
		}
		else
		{
			line[--numchar] = '\0';
		}
		if(numchar>=len-1) 
		  break; 
	}
}


int main(int argc, char **argv){
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	char line2[5000];
	struct sockaddr_in serveraddr;

	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(9874);
	serveraddr.sin_addr.s_addr=inet_addr("127.0.0.1");

	int e = connect(sockfd,(struct sockaddr*)&serveraddr,
					sizeof(struct sockaddr_in));
	if(e < 0)
	{
		printf("Some problems with connecting\n");
		return -1;
	}

	int shmid = shmget(IPC_PRIVATE, 5000, 0660); 
	char *line = (char *) shmat(shmid, NULL, 0); 
	
	// Wait for the username prompt
	bzero(line2, 5000);
	int n = recv(sockfd,line2,5000,0);
	printf("%s\n", line2);
	fflush(stdout);
	
	// Grab username and send back to the server
	mygetline(line,5000); 
	send(sockfd, line, strlen(line), 0);
	
	pid_t pid = fork(); 
	// listens to socket 
	if(pid == 0)
	{
		while (1)
		{
			bzero(line2, 5000);
			int n = recv(sockfd,line2,5000,0);
			printf("%s\n",line2);
			fflush(stdout); 
		}
	// listens to keyboard 
	} 
	else
	{ 
		struct termios tio; 
		tcgetattr(0,&tio); 
		tio.c_lflag &= ~ICANON;
		tcsetattr(0,TCSANOW,&tio); 
		printf(">");
		fflush(stdout);
		mygetline(line,5000);

		while(strncmp(line,"exit",4)!=0)
		{
			send(sockfd,line,strlen(line),0);
			printf(">");
			fflush(stdout);
			mygetline(line,5000); 
		}

		tcgetattr(0,&tio); 
		tio.c_lflag |= ICANON;
		tcsetattr(0,TCSANOW,&tio); 
		shmdt(line); 
		shmctl(shmid, IPC_RMID, NULL); 
		kill(pid,SIGTERM); 
	}

	return 0;
}




