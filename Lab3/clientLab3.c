#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<netdb.h>

void error(const char *msg)
{
	perror(msg);
	exit(1);
}


struct UdpMsg {
	uint32_t magicNumber; // The magicNumber 0x4A6F7921.
	uint16_t tcpPort;	// Here we'll just use this unsigned 16 bit since we are just having to extract it from the server side, and not this side.
	uint8_t GID;	// The ID of the group.
}__attribute__((__packed__));

struct UdpReply { // For when maybe told to wait, maybe error msgs.
	uint32_t magicNumber; // The magicNumber 0x4A6F7921.
	uint8_t GID;	// The GID of the server.
	uint8_t Ph;	// The most significant byte of port.
	uint8_t Pl;	// The least significant byte of port.

}__attribute__((__packed__));

struct UdpWaitReply { // For when told to wait.
	uint32_t magicNumber; // The magicNumber 0x4A6F7921.
	uint8_t GID;	// The GID of the server.
	uint16_t waitPort;	// Port we are told to wait at.

}__attribute__((__packed__));

struct UdpInvite { // For when given chat partner info.
	uint32_t magicNumber; // The magicNumber 0x4A6F7921.
	struct in_addr waitingIP; // IP addr of waiting client.
	uint16_t waitingPort;	// Port of client who is waiting to chat.
	uint8_t GID;	// The GID of the server.
	
}__attribute__((__packed__));

int main(int argc, char *argv[])
{
	if (argc < 4) {
		printf("please give these 3 arguments: ServerName ServerPort MyPort\n");
		exit(1);
	}
	
	short serverPort = atoi(argv[2]);
	short tcpPort = atoi(argv[3]);
	uint8_t GID = 21;
	
	printf("serverPort is %s\n", argv[1]);
	
	// Data members
	int sockfd, portNum, newsockfd, socketReadErrorFlag, recvlen;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clientLength = sizeof(cli_addr);
	char buffer[512]; // Probably have to change this.
	
	portNum = 10115; // Need to change.
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	else 
		printf("Socket seem to create sucessfully~\n");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	struct hostent *hp;     /* host information */
	struct sockaddr_in servaddr;    /* server address */
	
	/* fill in the server's address and data */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(portNum);
	
	/* look up the address of the server given its name */
	hp = gethostbyname(argv[1]);
	if (!hp) {
		fprintf(stderr, "could not obtain address of %s\n", argv[1]);
	}
	
	/* put the host's address into the server address structure */
	memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

	struct UdpMsg msg2Server; // Message to send to server.

	msg2Server.magicNumber = htonl((uint32_t)(0x4A6F7921));
	msg2Server.tcpPort = htons((uint16_t)(tcpPort));	// Important here that we use the network byte order.
	msg2Server.GID = GID;
	
	
	/* send a message to the server */
	if (sendto(sockfd, (void *) &msg2Server, 7, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("sendto failed");
		return 0;
	}
	
	struct UdpReply waitOrError;
	struct UdpWaitReply udpWaitInfo;
	struct UdpInvite chatInstr;
	
	int amTServer = 0;
	int amTClient = 0;
	
	listen(sockfd,5);
	clientLength = sizeof(cli_addr);
	recvlen = recvfrom(sockfd, buffer, 512, 0, (struct sockaddr *)&cli_addr, &clientLength);
	if (recvlen > 0) {
		printf("recvlen %i\n", recvlen);
		if (recvlen == 7) { // This is either an error or a wait request
			memcpy(&waitOrError, buffer, 7);
			printf("error or reply magicNumber in non-network Byte Order: %x\n", ntohl(waitOrError.magicNumber));
			printf("error or reply Ph: %i\n", waitOrError.Ph);
			
			// Checks Magic Number.
			if(ntohl(waitOrError.magicNumber) != (uint32_t)(0x4A6F7921)){ //0x4A6F7921
				printf("Magic Number Returned from Server is incorrect. Exiting Program.\n");
				return(1);
			}
			
			if(waitOrError.Ph == 0) { // This means this is an ERROR message.
				printf("There was an error in our request. \n");
				printf("Error Code(waitOrError.Pl) is: %x\n", waitOrError.Pl);
			}
			else { // We received a message to tell us to wait.
				printf("The request was successful, we are told to wait for a partner. \n");
				memcpy(&udpWaitInfo, buffer, 7);
				printf("wait message magicNumber %x\n", udpWaitInfo.magicNumber);
				printf("wait message GID %i\n", udpWaitInfo.GID);
				printf("wait message waitPort %i\n", udpWaitInfo.waitPort);
				printf("wait message waitPort non-Network byte order %i\n", ntohs((uint16_t)(udpWaitInfo.waitPort)));
				
				amTServer = 1;
			}
			printf("waitOrError.magicNumber: %x\n", waitOrError.magicNumber);
			
		}
		else if (recvlen > 7) { // This is an invite ?
			printf("Recieved message longer than 7 bytes, suspect is a chat invite. \n");
			memcpy(&chatInstr, buffer, 11);
			printf("chat invite magicNumber %x\n", chatInstr.magicNumber);
			printf("chat invite magicNumber in non-network Byte Order: %x\n", ntohl(chatInstr.magicNumber));
			// Checks Magic Number.
			if(ntohl(chatInstr.magicNumber) != (uint32_t)(0x4A6F7921)){ //0x4A6F7921
				printf("Magic Number Returned from Server is incorrect. Exiting Program.\n");
				return(1);
			}
			
			printf("chat invite waitingIP non-Network byte order %s\n", inet_ntoa(chatInstr.waitingIP));
			printf("chat invite waitingPort %i\n", chatInstr.waitingPort);
			printf("chat invite waitingPort non-Network byte order %i\n", ntohs((uint16_t)(chatInstr.waitingPort)));
			printf("GID %i\n", chatInstr.GID);
			
			amTClient = 1;
		}
		else {
			printf("There are errors with the response that came back. Length is shorter than 7. \n");
		}
				
	}
	close(sockfd);
	
	char endMessage[14] = "Bye Bye Birdie";
	
	if(amTServer > 0) {		// If I am the Server.
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			error("ERROR opening socket");
		else 
			printf("Socket seem to create sucessfully~\n");
		
		// clear address structure
		bzero((char *) &serv_addr, sizeof(serv_addr));
		
		portNum = udpWaitInfo.waitPort;
		// setup host addr structure for us in bind call
		// Server byte order
		serv_addr.sin_family = AF_INET;
		
		// Convert short integer value for port must be converted into network byte order
		serv_addr.sin_port = portNum;
		// serv_addr.sin_port = htons(portNum);
		
		// automatically be filled with current host's IP address
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		// bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
		// bind() passes file descriptor, the address structure, 
		// and the length of the address structure
		// This bind() call will bind  the socket to the current IP address on port, portNum
		if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) {
				error("ERROR on binding\n");
				exit(1);
			}
		
		// This listen() call tells the socket to listen to the incoming connections.
		// The listen() function places all incoming connection into a backlog queue
		// until accept() call accepts the connection.
		// Here, we set the maximum size for the backlog queue to 5.
		listen(sockfd,5);			
				
		// The accept() call actually accepts an incoming connection
		//clientLength = sizeof(cli_addr);
		printf("Listening for client...\n");
		
		/* This accept() function will write the connecting client's address info 
		   into the the address structure and the size of that structure is clientLength.
		   The accept() returns a new socket file descriptor for the accepted connection.
		   So, the original socket file descriptor can continue to be used 
		   for accepting new connections while the new socker file descriptor is used for
		   communicating with the connected client. */
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clientLength);
		if (newsockfd < 0)  {
			error("ERROR on accept");
			exit(1);
		}
		
		bzero(buffer,256);

		while(1) {
			// Initial reply to client
			socketReadErrorFlag = recv(newsockfd, buffer, 256, 0);
			if (socketReadErrorFlag < 0) 
				error("ERROR during recv() from socket");
			
			char byebye[14]; 
			memcpy(byebye, buffer, 14);
			
			printf("Client said:\n%s", buffer);
			if(strcmp(byebye, endMessage) == 0){
				close(newsockfd);
				return 0;
			}
			
			printf("Please enter msg to client: \n");
			bzero(buffer, 256);
			fgets(buffer, 256, stdin);
			if (send(newsockfd, buffer, sizeof(buffer), 0) < 0){
				error("ERROR happened during send()\n");
			}
			memcpy(byebye, buffer, 14);
			if(strcmp(byebye, endMessage) == 0){
				return 0;
			}
			
			bzero(buffer,256);
		}
	}
	else if (amTClient > 0) { // If I am the Client
		int sockfd2, recv2;
		struct sockaddr_in serv_addr2;
		struct hostent *hp2;
		
		portNum = chatInstr.waitingPort;
	
		/* socket: create the socket */
		sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd2 < 0) 
			error("ERROR opening socket");
	
		/* gethostbyname: get the server's DNS entry */
		hp2 = gethostbyname(inet_ntoa(chatInstr.waitingIP));
		if (hp2 == NULL) {
			fprintf(stderr,"ERROR, no such host as %s\n", chatInstr.waitingIP);
			exit(0);
		}
	
		/* build the server's Internet address */
		bzero((char *) &serv_addr2, sizeof(serv_addr2));
		serv_addr2.sin_family = AF_INET;
		bcopy((char *)hp2->h_addr, (char *)&serv_addr2.sin_addr.s_addr, hp2->h_length);
		serv_addr2.sin_port = portNum;
	
		/* connect: create a connection with the server */
		if (connect(sockfd2, (struct sockaddr *)&serv_addr2, sizeof(serv_addr2)) < 0) 
		error("ERROR connecting");
	
		while(1) {
			char byebye[14]; 
			/* get message line from the user */
			printf("Please enter msg:\n");
			bzero(buffer, 256);
			fgets(buffer, 256, stdin);
		
			/* send the message line to the server */
			if(send(sockfd2, buffer, 256, 0) < 0){
				error("ERROR happened during send()\n");
			}
			memcpy(byebye, buffer, 14);
			if(strcmp(byebye, endMessage) == 0){
				return 0;
			}
		
			/* print the server's reply */
			bzero(buffer, 256);
			if(recv(sockfd2, buffer, 256, 0) < 0) {
				error("ERROR happened during recv()\n");
			}
			
			printf("Server said: \n%s", buffer);
			
			memcpy(byebye, buffer, 14);
			
			if(strcmp(byebye, endMessage) == 0){
				close(sockfd2);
				return 0;
			}
			
		}
	}
}