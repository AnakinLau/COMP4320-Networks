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

/**
Incoming Msg format:

|Magic# 0x4A6F7921| TML | GID | Checksum | RequestID | Length1 | Hostname 1 | ... | Ln | Hn |
| 	 4 bytes	  |  2  |  1  |     1    |      1    |    1    |  variable  | ... | 1  |var |
**/

/** 
The first we need to parse the first 5 elements of the incoming request.
These are everything but the Hostnames(Hn) and Hostname lengths (Ln).
**/
struct requestHeader {
	uint32_t magicNumber;
	uint16_t TML;
	uint8_t GID;
	uint8_t checksum;
	uint8_t requestID;
}__attribute__((__packed__));

struct requestContent {
		char hostInfo[65537];
		//char ipArray[512];
		struct in_addr ipArray[512];
		int reqVarCount;
}__attribute__((__packed__));

struct failResponse {
		uint32_t magicNumber;
		uint16_t TML;
		uint8_t GID;
		uint8_t checksum;
		uint8_t BEC; // Byte Error Code.
}__attribute__((__packed__));

/**
	Compute the checksum of an item.
	I used the following link as a guide
	http://www.faqs.org/rfcs/rfc1071.html
**/
uint8_t checksum(const void* memory, int length) {

  uint8_t* mem = (uint8_t*) memory; // Casting each byte in memory to their own byte
  int sum = 0;
  int i;
  for (i = 0; i < length; i++) {
    sum += *(mem + i); 
    while (sum > 255)
      sum = (sum & 0xFF) + (sum >> 8); 
  }
  
  return ~sum; 
}
/**Resolves IP Address from Host/URL**/
int hostname_to_ip(char *  , char *);

int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

int main(int argc, char *argv[])
{
	int consecErrorCounter = 0;
	// Loop should start here (?)
	
	int sockfd, portNum, newsockfd, socketReadErrorFlag, recvlen;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clientLength = sizeof(cli_addr);
	char buffer[65536]; // Max size is 2^16
	
    if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
	// Create a socket
	// socket(int domain, int type, int protocol)
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	else 
		printf("Socket seem to create sucessfully~\n");
	
	// clear address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
	
	portNum = atoi(argv[1]);

	// setup host addr structure for us in bind call
	// Server byte order
	serv_addr.sin_family = AF_INET;
	
	// Convert short integer value for port must be converted into network byte order
	serv_addr.sin_port = htons(portNum);
	
	// automatically be filled with current host's IP address
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
    // bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
    // bind() passes file descriptor, the address structure, 
    // and the length of the address structure
    // This bind() call will bind  the socket to the current IP address on port, portNum
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr)) < 0) 
			error("ERROR on binding\n");

    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    // Here, we set the maximum size for the backlog queue to 5.
    listen(sockfd,5);			
			
    clientLength = sizeof(cli_addr);
	 
	while(1) {
	
		//int recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict *src_len)
		recvlen = recvfrom(sockfd, buffer, 65536, 0, (struct sockaddr *)&cli_addr, &clientLength);
		if (recvlen > 0) {
			
			buffer[recvlen] = 0;
			printf("received message: \"%s\"\n", buffer);
						
			struct requestHeader inRequest; //This is the message from the client
			inRequest.magicNumber = ntohl(*(uint32_t*)(buffer));
			inRequest.TML = ntohs(*(uint16_t*)(buffer + 4));
			inRequest.GID = (uint8_t)(buffer[6]);
			inRequest.checksum = (uint8_t)(buffer[7]);
			inRequest.requestID = (uint8_t)(buffer[8]);
	
			/**
				TODO: Create code to check validity of incoming magic number.
				If invalid, should jump to error path.
			**/
			struct failResponse failPacket; // We will create one regardless if failed or not. Just for convinenence sake.
			failPacket.BEC = 0; // Start out as 0000 0000
			
			// Bad/incorrect length or TML.
			if(inRequest.TML != recvlen) {
				printf("inRequest.TML != recvlen \n");
				printf("inRequest.TML = %i\n", inRequest.TML);
				printf("recvlen = %i\n", recvlen);
				failPacket.BEC = failPacket.BEC + 1; // 1 is 0000 0001
			}
			// Bad/incorrect magic number.
			if(inRequest.magicNumber != 1248819489) {
				failPacket.BEC = failPacket.BEC + 2; // 2 is 0000 0010
				printf("inRequest.magicNumber != 1248819489 \n");
				printf("inRequest.magicNumber = %i\n", inRequest.TML);
			}
			
			printf("magicNumber in hex: %lX\n", inRequest.magicNumber); 
			printf("magicNumber in dec: %i\n", inRequest.magicNumber); 
			printf("TML: %i\n", inRequest.TML); 
			printf("RequestID: %i\n", inRequest.requestID);
			printf("GID: %i\n", inRequest.GID);
			printf("checksum: %i\n", inRequest.checksum);		
		

			int reqContentLen = inRequest.TML - 9; //Length of the contents, everything but the header
			
			int checksumResult = checksum(&buffer, inRequest.TML);
			printf("checksumResult: %i\n", checksumResult);
			
			// Checksum is bad/incorrect.
			if(checksumResult != 0) 
			{
				failPacket.BEC = failPacket.BEC + 4; // 4 is 0000 0100
				//Go to Sad pat
			}
			
			printf("request Content Len: %i\n", reqContentLen);
			
			if(!(failPacket.BEC > 0)) { // No error exists for the request.
				
				consecErrorCounter = 0; // Reset number of consecutive errors.
	
				
				struct requestContent inContent;
		
				// Copies the remaining bytes into another array
				memcpy(inContent.hostInfo, &buffer[9], reqContentLen);
				
				int urlCopyItor = 0; // Used to iterate the array
				inContent.reqVarCount = 0; // Set the number of processed vars to 0.
				while(urlCopyItor < reqContentLen) 
				{
					//run thur each and add the length in urlCopyItor
					char urlString[512];
					char urlLength;
					urlLength = inContent.hostInfo[urlCopyItor]; // Set it first to the length of the var that follows
					urlCopyItor = urlCopyItor + 1; //For the one byte var length
					memcpy(urlString, &inContent.hostInfo[urlCopyItor], (uint8_t)urlLength);
					urlCopyItor = urlCopyItor + (uint8_t)urlLength;

					int ipInsertPlace = 4 * inContent.reqVarCount;
					char *hostname = urlString;
					char ip[100];
			
					hostname_to_ip(hostname , ip);
					printf("%s resolved to %s" , hostname , ip);
					
					printf("\n");
					
					inet_aton(ip, &inContent.ipArray[ipInsertPlace]);
					
					inContent.reqVarCount = inContent.reqVarCount + 1; // Increase our count of number of host/ip addresses.
					
					bzero(urlString,512);
				}
				
				struct successResponse {
					uint32_t magicNumber;
					uint16_t TML;
					uint8_t GID;
					uint8_t checksum;
					uint8_t requestID;
					struct in_addr IPs[inContent.reqVarCount];
				}__attribute__((__packed__));
				
				struct successResponse returnMsg;
				returnMsg.magicNumber = htonl(1248819489); // The magic number
				returnMsg.TML = htons(9 + (inContent.reqVarCount * 4));
				returnMsg.GID = inRequest.GID; 
				returnMsg.checksum = 0; // Checksum temporaily set to 0 for calculation.
				returnMsg.requestID = inRequest.requestID;

				// For some reason I was only able to copy the array of IPs this way. But there are probably better ways I wasn't able to think of...
				int IPinsertIter = 0;
				while (IPinsertIter < inContent.reqVarCount){
					returnMsg.IPs[IPinsertIter] = inContent.ipArray[IPinsertIter];
					IPinsertIter = IPinsertIter + 1;
				}
				
				printf("checksum for valid return msg is: %i\n", checksum(&returnMsg, returnMsg.TML));
				returnMsg.checksum = checksum(&returnMsg, returnMsg.TML);
				printf("checksum validity check: %i\n", checksum(&returnMsg, returnMsg.TML));

				sendto(sockfd, (void *) &returnMsg, returnMsg.TML, 0, (struct sockaddr *)&cli_addr, clientLength);
			}
			
			// Below is when request has errors.
			else {
				printf("failPacket.BEC: %i\n", failPacket.BEC);
				
				consecErrorCounter = consecErrorCounter + 1; // Increment number of consecutive errors.
				
				failPacket.magicNumber = htonl(1248819489); // The magic number
				failPacket.TML = htons(9);
				failPacket.GID = inRequest.GID;
				failPacket.checksum = 0; // Temporarily set checksum to 0 for calculation.
				// failPacket.BEC is already calculated as we got to this point.
				
				printf("checksum for error msg is: %i\n", checksum(&failPacket, 9));
				failPacket.checksum = checksum(&failPacket, 9);
				printf("checksum for error msg is: %i\n", checksum(&failPacket, 9));
				
				sendto(sockfd, (void *) &failPacket, failPacket.TML, 0, (struct sockaddr *)&cli_addr, clientLength);
				
				if (consecErrorCounter > 7) {
					printf("Request Error happened more than 7 times. Program will now quit.");
					exit(0);
				}
			}
			
		}
	}
    close(sockfd);
    return 0;
}