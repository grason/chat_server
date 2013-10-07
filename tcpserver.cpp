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
#define LIMIT 5000

#define CL_LIST 0
#define GR_LIST 1
#define ADMIN 2
#define MESSAGE_ALL 3
#define MESSAGE_DIR 4
#define CR_GROUP 5
#define JN_GROUP 6
#define MESSAGE_GRP 7

char* commands[] = {"/list", "/list_group", "/make_admin", "/message_all", "/message_direct", "/create", "/join", "/message_group"};

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
    serveraddr.sin_port=htons(9876);
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
	int first_user = 1;
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
		    
		    if(first_user)
		    {
			c1->admin = 1;
			first_user = 0;
		    }

		    strcpy(c1->username, line);
		    printf("%s\n", c1->username);
		    
                    //Add the client to our list of clients
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
		    
		    if(!strcmp(token, commands[CL_LIST]))
		    {
			bzero(line, LIMIT);
			strcpy(line, "Current user list:\n");
			for(vector<client*>::iterator it = clients.begin(); it != clients.end(); it++)
			{
			    strcat(line,(*it)->username);
			}
			
			send(i,line,strlen(line),0);
		    }
		    else if(!strcmp(token, commands[GR_LIST]))
		    {
			bzero(line, LIMIT);
			strcpy(line, "Current group list:\n");
			for(vector<group*>::iterator it = groups.begin(); it != groups.end(); it++)
			{
			    strcat(line,(*it)->group_name);
			    strcat(line, "\n");
			}
			
			send(i,line,strlen(line),0);
		    }
		    else if(!strcmp(token, commands[ADMIN]))
		    {
			bzero(line, LIMIT);
			
			token = strtok(NULL, " ");
			
			if(token != NULL)
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

			    // If the user was an admin...
			    if(is_admin)
			    {
				// Put the sender's username in the message
				for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
				{
				    if(strcmp((*it)->username, token))
				    {
					// Set users admin bool and send him a message
					(*it)->admin = 1;
					strcpy(line, "You have been made an admin\n");
					send((*it)->socket, line, strlen(line), 0);
					
					// Set the return information for the client
					strcpy(line, "User has been successfully promoted to admin status.\n");
					break;
				    }
				}
			    }
			    // Else, send the client an error stating they are not an admin
			    else
				strcpy(line, "You are not an admin!");
			}
			// Else, print out an error stating a username is needed
			else
			    strcpy(line, "No username submitted!");

			send(i, line, strlen(line), 0);
		    }
		    else if(!strcmp(token, commands[MESSAGE_ALL]))
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
		    else if(!strcmp(token, commands[MESSAGE_DIR]))
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
		    else if(!strcmp(token, commands[CR_GROUP]))
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
				if(!strcmp((*it)->group_name, token))
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
				    strcpy(g->group_name, token);
				    groups.push_back(g);
				    
				    // Send confirmation message
				    strcpy(line, "Group \"");
				    strcat(line, token);
				    strcat(line, "\" has been created.\n");
			  	}
				else
				    strcpy(line, "You do not have admin rights.\n");

				send(i, line, strlen(line), 0);

			    }
			}
			else
			{
			    strcpy(line, "Group name is missing.\n");
			    send(i, line, strlen(line), 0);
			}
		    }
		    else if(!strcmp(token, commands[JN_GROUP]))
		    {
			token = strtok(NULL, " ");
			bzero(line, LIMIT);
			
			// If a group name was provided...
			if(token != NULL)
			{
			    int is_group = 0;
			    // For each of our clients...
			    for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
			    {
				// If this is the user...
				if(i == (*it)->socket)
				{
				    // Find the group that the user wants to join...
				    for(vector<group*>::iterator gr = groups.begin(); gr != groups.end(); ++gr)
				    {
					if(!strcmp((*gr)->group_name, token))
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
				strcpy(line, "Group could not be found.\n");
			}
			else
			    strcpy(line, "Group name is missing.\n");
			
			send(i, line, strlen(line), 0);
		    }
		    else if(!strcmp(temp, commands[MESSAGE_GRP]))
		    {
			bzero(line, LIMIT);
			token = strtok(NULL, " ");

			if(token != NULL)
			{
			    client* c;

			    int is_in_group = 0;
			    // Find the user and grab its admin value...
			    for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
			    {
				if(i == (*it)->socket)
				{	
				    for(vector<char*>::iterator gr = (*it)->groups.begin(); gr != (*it)->groups.end(); ++gr)
				    {
					// If the group name is the one passed
					if(!strcmp(*gr, token))
					{
					    is_in_group = 1;
					    break;
					}
				    }
				    // If we get here, we've either found the group or not...
				    break;
				}
	       		    }
       			
			    if(is_in_group)
			    {
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

				if(count < 1)
				{
				    
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
		    else if(!strcmp(token, commands[MESSAGE_DIR]))
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
		    else if(!strcmp(token, commands[CR_GROUP]))
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
				if(!strcmp((*it)->group_name, token))
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
				    strcpy(g->group_name, token);
				    groups.push_back(g);
				    
				    // Send confirmation message
				    strcpy(line, "Group \"");
				    strcat(line, token);
				    strcat(line, "\" has been created.\n");
			  	}
				else
				    strcpy(line, "You do not have admin rights.\n");

				send(i, line, strlen(line), 0);

			    }
			}
			else
			{
			    strcpy(line, "Group name is missing.\n");
			    send(i, line, strlen(line), 0);
			}
		    }
		    else if(!strcmp(token, commands[JN_GROUP]))
		    {
			token = strtok(NULL, " ");
			bzero(line, LIMIT);
			
			// If a group name was provided...
			if(token != NULL)
			{
			    int is_group = 0;
			    // For each of our clients...
			    for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
			    {
				// If this is the user...
				if(i == (*it)->socket)
				{
				    // Find the group that the user wants to join...
				    for(vector<group*>::iterator gr = groups.begin(); gr != groups.end(); ++gr)
				    {
					if(!strcmp((*gr)->group_name, token))
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
				strcpy(line, "Group could not be found.\n");
			}
			else
			    strcpy(line, "Group name is missing.\n");
			
			send(i, line, strlen(line), 0);
		    }
		    else if(!strcmp(temp, commands[MESSAGE_GRP]))
		    {
			bzero(line, LIMIT);
			token = strtok(NULL, " ");

			if(token != NULL)
			{
			    char username[USERNAME_BOUND];
			    char groupname[GROUPNAME_BOUND];
			    
			    // Save the group name...
			    strcpy(groupname, token);
			    int is_in_group = 0;
			    // Find the user and grab its admin value...
			    for(vector<client*>::iterator it = clients.begin(); it != clients.end(); ++it)
			    {
				if(i == (*it)->socket)
				{	
				    for(vector<char*>::iterator gr = (*it)->groups.begin(); gr != (*it)->groups.end(); ++gr)
				    {
					// If the group name is the one passed
					if(!strcmp(*gr, token))
					{
					    strcpy(username, (*it)->username);
					    is_in_group = 1;
					    break;
					}
				    }
				    // If we get here, we've either found the group or not...
				    break;
				}
	       		    }
       			
			    if(is_in_group)
			    {
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

				// If there was no message sent...
				if(count < 1)
				{
				    strcpy(line, "Message text missing!\n");
				    send(i, line, strlen(line), 0);
				}
				else
				{
				    // Find the group name specified..
				    for(vector<group*>::iterator gr = groups.begin(); gr != groups.end(); ++gr)
				    {
					// If the group name is the one passed
					if(!strcmp((*gr)->group_name, groupname))
					{
					    // For each of the clients within that group...
					    for(vector<client*>::iterator mem = (*gr)->members.begin(); mem != (*gr)->members.end(); ++mem)
					    {
						// If it is not the client who sent the message...
						if(strcmp((*mem)->username, username))
						{
						    // SEND!
						    send((*mem)->socket, line, strlen(line), 0);
						}
					    }
					}
				    }
				}
			    }
			    // The user is not a part of the requested group...
			    else
			    {
				strcpy(line, "You are not a part of the group: \"");
				strcat(line, token);
				strcat(line, "\"!");
				send(i, line, strlen(line), 0);
			    }
			}
			else
			{
			    strcpy(line, "Group name missing!\n");
		 	    send(i, line, strlen(line), 0);
			}
		    }
			}
    
    return 0;
}






