/**
 Name: Atefe Mosayebi
 Student ID: 1461654
 Program Assignment 2: Client
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#define PORT     8080
#define MAXLINE 1024

#define STARTID "FFFF"
#define ENDID "FFFF"
#define CLIENTID "FF"
#define LENGTH 32

#define ACCESS_PER "FFF8"
#define NOT_PAID "FFF9"
#define NOT_EXIST "FFFA"
#define DIF_TECH "FFFC"
#define ACCESS_OK "FFFB"

#define RETRY 3
#define TIMEOUT 3

// struct -  PermissionVerification packet (client sends to server and server sends to client)
struct PermissionVerification {
    char start[5];
    char clientID[3];
    char type[5] ;
    int segmentNo;
    int length;
    int technology;
    long long subscriberNo;
    char end[5];
};

// creates PermissionVerification to send to server
struct PermissionVerification generatePermission(int segmentNo, int technology, long long subscriberNo) {
    struct PermissionVerification permission;
    
    strcpy(permission.start, STARTID);
    strcpy(permission.end, ENDID);
    strcpy(permission.clientID, CLIENTID);
    permission.length = LENGTH;
    strcpy(permission.type, ACCESS_PER);
    permission.segmentNo = segmentNo;
    permission.technology = technology;
    permission.subscriberNo = subscriberNo;
    
    return permission;
}

// Driver code
int main() {
    
    int sockfd;
    struct sockaddr_in servaddr, from;
    int fromlen;
    struct  PermissionVerification packets[5];  // didn't use index 0.
    int segmentNo = 1;
    struct PermissionVerification receivedPacket;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    socklen_t fromLen = sizeof(from);

    memset(&servaddr, 0, sizeof(servaddr));
    
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    
    // provide 3 second timeout for the socket
    struct timeval tv;
    tv.tv_sec = TIMEOUT;  // 3 Secs Timeout
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
    
    // create packet 1: not paid packet
    packets[1] = generatePermission(1, 3, 4086668821);
    
    // create packet 2: diffrent technology
    packets[2] = generatePermission(2, 3, 4085546805);
    
    // create packet 3: not exist in database
    packets[3] = generatePermission(3, 3, 4081111111);

    // create packet 4: correct packet that gets the verification
    packets[4] = generatePermission(4, 2, 4086808821);

    int i = 1;
    // send 4 packages
    while (i < 5) {
        
        int retry = 0;
        int n = 0;
        struct PermissionVerification packet = packets[i];
        
        while(n <= 0 && retry < RETRY) {
    
            sendto(sockfd, &packet, sizeof(struct PermissionVerification), 0,
                   (const struct sockaddr *) &servaddr, sizeof(servaddr));
            printf("packet %d was sent to the server.\n", packet.segmentNo);
           
            n = recvfrom(sockfd, (struct PermissionVerification *) &receivedPacket,
                         sizeof(struct PermissionVerification),
                         0 ,(struct sockaddr *) &from, &fromlen);
            
            if (n <= 0 ) {
                
                printf("NO RESPONSE from server for 3 seconds. Sending the packet %d again.\n\n", packet.segmentNo);
                retry++;
                
            } else if (strcmp(receivedPacket.type, NOT_PAID) == 0) { // not paid
                
                printf("packet with segment number %d and %ul subscriber number was NOT PAID\n",
                       receivedPacket.segmentNo, receivedPacket.subscriberNo);
                
            } else if (strcmp(receivedPacket.type, DIF_TECH) == 0) { // diff tech
                
                printf("packet with segment number %d and %ul subscriber number uses DIFFRENT TECHNOLOGY\n",
                       receivedPacket.segmentNo, receivedPacket.subscriberNo);
                
            } else if (strcmp(receivedPacket.type, NOT_EXIST) == 0) { // not exist
                
                printf("packet with segment number %d and %ul subscriber number was NOT FOUND IN DATABSE\n",
                       receivedPacket.segmentNo, receivedPacket.subscriberNo);
                
            } else if (strcmp(receivedPacket.type, ACCESS_OK) == 0) {    // verified
                
                printf("packet with segment number %d  and %ul subscriber number was GRANTED PERMISSION!!!\n",
                       receivedPacket.segmentNo, receivedPacket.subscriberNo);
            }
   
        }   // end of while
        
        // If resend the packet more than 3 times
        if (retry >= 3) {
            printf("\nSERVER DOES NOT RESPOND \n\n");
            exit(0);
        }
        i++;
        printf("-----------------------------------------------\n\n");
    }   // end of for
    
    close(sockfd);
    return 0;
}
