#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <utility>
#include <vector>

#define USERNAME_BOUND 20
#define GROUPNAME_BOUND 20
#define LIMIT 500

using namespace std;

struct client 
{
	char username[USERNAME_BOUND];
	int admin;
	int socket;
	vector<char*> groups;
};

struct group
{
	char group_name[GROUPNAME_BOUND];
	vector<client*> members;
};

int main(int argc, char **argv)
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	char line[LIMIT];
	int len;
	vector<client*> clients; 
	vector<group*> groups;
	
	//Create an empty set of sockets
	fd_set sockets;
	FD_ZERO(&sockets);

	struct sockaddr_in serveraddr, clientaddr;
	  
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(9874);
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
					
					bzero(line, LIMIT);
					int n = recv(clientsocket, line, LIMIT, 0);
					
					client* c1 = new client();
					c1->socket = clientsocket;
					strcpy(c1->username, line);
					printf("%s\n", c1->username);
					//Add the client/socket pair to our list of clients
					clients.push_back(c1);
					
					//Add the socket to the set of sockets
					FD_SET(clientsocket, &sockets);
				} 
				// Else, we just need to handle input from i
				else 
				{
					int n;
					char* token;
					
					bzero(line, LIMIT);
					n = recv(i, line, LIMIT, 0);
					printf("Got %d bytes from client: %s\n",n,line);
					
					char temp[LIMIT];
					
					strcpy(temp, line);
					token = strtok(temp, " ");
					
					if(!strcmp(token, "/list"))
					{
						bzero(line, LIMIT);
						strcpy(line, "Current user list:\n");
						for(vector<client*>::iterator it = clients.begin(); it != clients.end(); it++)
						{
							strcat(line,(*it)->username);
						}
						
						send(i,line,strlen(line),0);
					}
					else if(!strcmp(token, "/message_all"))
					{
						// Create message text
						bzero(line, LIMIT);
						strcpy(line, "Sent from user ");
						
						// Put the sender's username in the message
						for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
						{
							if(i == (*it)->socket)
							{
								strcat(line, (*it)->username);
								strcat(line, ": ");
								break;
							}
						}
						
						int count = 0;
						token = strtok(NULL, " ");
						
						// For each token, concatenate it with line
						while(token != NULL)
						{
							strcat(line, " ");
							strcat(line, token);
							token = strtok(NULL, " ");
							count++;
						}
						
						// If there was not a second token, give error message
						if(count < 1)
						{
							strcpy(line, "Message text missing!");
							send(i, line, strlen(line), 0);
						}
						// Otherwise, send the message
						else
						{
							// For each client we have...
							for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
							{
								if(i != (*it)->socket)
									send((*it)->socket, line, strlen(line), 0);
							}
						}
					}
					else if(!strcmp(token, "/message_direct"))
					{
						// Create message text
						bzero(line, LIMIT);
						strcpy(line, "Sent from user ");
						
						// Put the sender's username in the message
						for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
						{
							if(i == (*it)->socket)
							{
								strcat(line, (*it)->username);
								strcat(line, ": ");
								break;
							}
						}
						
						token = strtok(NULL, " ");
						
						if(token == NULL)
						{
							strcpy(line, "Username missing");
							send(i, line, strlen(line), 0);
						}
						else
						{
							// Save the username 
							char name[USERNAME_BOUND];
							strcpy(name, token);
							
							int count = 0;
							token = strtok(NULL, " ");
							// For each token, concatenate it with line
							while(token != NULL)
							{
								strcat(line, " ");
								strcat(line, token);
								token = strtok(NULL, " ");
								count++;
							}
							
							// If no more tokens were found, print out an error message
							if(count < 1)
							{
								strcpy(line, "Message text missing!");
								send(i, line, strlen(line), 0);
							}
							// Else, send the message to the right client
							else
							{
								// For the client with username 'name', send the message
								for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
								{
									if(!strcmp((*it)->username, name))
									{
										send((*it)->socket, line, strlen(line), 0);
										break;
									}
								}
							}
						}
					}
					else if(!strcmp(line, "/create"))
					{
						int create_group = 1;
						
						// Grab the new group name
						token = strtok(NULL, " ");
						
						//If there was a groupname provided
						if(token != NULL)
						{
							// For all of the groups, check if the group already exists
							for(vector<group*>::iterator it = groups.begin(); it != groups.end(); ++it)
							{
								if(!strcmp((*it)->groupname, token))
								{
									// Send message to client and send create_group to false
									strcpy(line, "Group already exists.");
									send(i, line, strlen(line), 0);
									create_group = 0;
									break;
								}
							}
							
							// If we got through all the groups without finding a repeat,
							if(create_group) 
							{
								int is_admin = 0;
								// Find the user and grab its admin value...
								for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
								{
									if(i == (*it)->socket)
									{
										is_admin = (*it)->admin;
										break;
									}
								}
								
								// If the user has admin rights, create the group and push it in
								// our vector
								if(is_admin)
								{
									group* g = new group();
									strcpy(g->groupname, token);
									groups.push_back(g);
									
									// Send confirmation message
									strcpy(line, "Group \"");
									strcat(line, token);
									strcat(line, "\" has been created.\n");
									send(i, line, strlen(line), 0);
								}
								else
								{
									strcpy(line, "You do not have admin rights.\n");
									send(i, line, strlen(line), 0);
								}
							}
						}
						else
						{
							strcpy(line, "Group name is missing.\n");
							send(i, line, strlen(line), 0);
						}
					}
					else if(!strcmp(line, "/join"))
					{
						token = strtok(NULL, " ");
						bzero(line, LIMIT);
						
						// If a group name was provided...
						if(token != NULL)
						{
							// For each of our clients...
							for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
							{
								// If this is the user...
								if(i == (*it)->socket)
								{
									int is_group = 0;
									// Find the group that the user wants to join...
									for(vector<group*>::iterator gr = groups.begin(); gr != groups.end(); ++gr)
									{
										if(!strcmp((*gr)->groupname, token))
										{
											// Save the groupname in the client and push the client into the group
											((*it)->groups).push_back(token);
											((*gr)->members).push_back(*it);
											is_group = 1;
											break;
										}
									}
									// We've either found the group or it doesn't exist
									break;
								}
							}
							
							// If we found the group...
							if(is_group)
							{
								strcpy(line, "You have been added to the group \"");
								strcat(line, token);
								strcat(line, "\"\n");
							}
							else
							{
								strcpy(line, "Group could not be found.\n");
							}
						}
						else
						{
							strcpy(line, "Group name is missing.\n");
							send(i, line, strlen(line), 0);
						}
					}
				}	
			}	
		}
	}

	return 0;
}






