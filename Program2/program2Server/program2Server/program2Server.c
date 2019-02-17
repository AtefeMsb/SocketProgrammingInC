/**
 Name: Atefe Mosayebi
 Student ID: 1461654
 Program Assignment 2: Server
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

#define PORT     8080
#define MAXLINE 1024

#define STARTID "FFFF"
#define ENDID "FFFF"
#define CLIENTID "FF"
#define LENGTH 32

// type of packet
#define ACCESS_PER "FFF8"
#define NOT_PAID "FFF9"
#define NOT_EXIST "FFFA"
#define DIF_TECH "FFFC"
#define ACCESS_OK "FFFB"

#define RETRY 3
#define TIMEOUT 3

#define MAXB 32
#define MAXL 18
#define MAXD 3

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

// struct - subscriber informations
struct Subscriber {
    long long subscriberNo;
    int technology;
    int paid;
};

// creates PermissionVerification to send to client
struct PermissionVerification generateVerification(char type[], int segmentNo, int technology, long long subscriberNo) {
    struct PermissionVerification verification;
    
    strcpy(verification.start, STARTID);
    strcpy(verification.end, ENDID);
    strcpy(verification.clientID, CLIENTID);
    verification.length = LENGTH;
    strcpy(verification.type, type);
    verification.segmentNo = segmentNo;
    verification.technology = technology;
    verification.subscriberNo = subscriberNo;
 
    return verification;
}

// Driver code
int main() {
    int sockfd;
    struct sockaddr_in serverAddr, from;
    unsigned int len;
    int i = 0;
    struct Subscriber subsciberList[3];
    long long int singleLine[MAXB] = {0};
    struct Subscriber subscriber;
    int fromlen, n;
    struct PermissionVerification receivedPacket;
    struct PermissionVerification sentPacket;
    
    socklen_t fromLen = sizeof(from);
    
    
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    // open the database and read the information into subsriber struct
    FILE *fp = fopen("verificationDatabase.txt", "r");
    if (fp == 0)  {
        fprintf(stderr, "Failed to open verificationDatabase.txt \n");
        return 1;
    }
    while (i < MAXL && fgets (singleLine, MAXB - 1, fp)) {
        if (sscanf (singleLine, "%lld %d %d",
                    &subsciberList[i].subscriberNo, &subsciberList[i].technology, &subsciberList[i].paid) == 3)
            i++;
    }
    fclose(fp);
 
    // Creating UDP socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Filling server information
    serverAddr.sin_family    = AF_INET; // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    bzero(&serverAddr.sin_zero, 8); // padding
    
    len = sizeof(struct sockaddr_in);
    
    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&serverAddr, len) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server is running and listening on port %d ...\n", PORT);
    printf("-----------------------------------------------\n\n");
    
    
    while (1) {
        
        int isSubscriber = 0;
        n = recvfrom(sockfd, (struct PermissionVerification *) &receivedPacket, sizeof(struct PermissionVerification),0 , (struct sockaddr *) &from, &fromLen);
        printf("Segment Number %d was received from client. \n", receivedPacket.segmentNo);
        
        // Check the received packet against database and fetch the record
        for(int i = 0; i < sizeof(subsciberList); i++) {
            if (receivedPacket.subscriberNo == subsciberList[i].subscriberNo) {
                subscriber = subsciberList[i];
                isSubscriber = 1;
                break;
            }
        }
        
        // subscriber does not exist
        if (isSubscriber == 0) {
            
            sentPacket = generateVerification(NOT_EXIST, receivedPacket.segmentNo,
                                              receivedPacket.technology, receivedPacket.subscriberNo);
            printf("User NOT EXIST in the database:: %ul\n\n", receivedPacket.subscriberNo);
            sendto(sockfd, &sentPacket, sizeof(struct PermissionVerification), 0, (const struct sockaddr *) &from, sizeof(from));

            
        } else if(subscriber.technology != receivedPacket.technology) { // exist but diffrent technology
            
            sentPacket = generateVerification(DIF_TECH, receivedPacket.segmentNo,
                                              receivedPacket.technology, receivedPacket.subscriberNo);
            printf("User is using DIFFRENT TECHNOLOGY:: %ul\n\n", receivedPacket.subscriberNo);
            sendto(sockfd, &sentPacket, sizeof(struct PermissionVerification), 0, (const struct sockaddr *) &from, sizeof(from));

    
        } else if (subscriber.paid == 0) { // subscriber has not paid
            
            sentPacket = generateVerification(NOT_PAID, receivedPacket.segmentNo,
                                              receivedPacket.technology, receivedPacket.subscriberNo);
            printf("User did NOT PAID:: %ul\n\n", receivedPacket.subscriberNo);
            sendto(sockfd, &sentPacket,
                   sizeof(struct PermissionVerification), 0,
                   (const struct sockaddr *) &from, sizeof(from));
        } else {
            
            sentPacket = generateVerification(ACCESS_OK, receivedPacket.segmentNo,
                                              receivedPacket.technology, receivedPacket.subscriberNo);
            printf("User is OKAY to connect:: %ul\n\n", receivedPacket.subscriberNo);
            sendto(sockfd, &sentPacket, sizeof(struct PermissionVerification), 0, (const struct sockaddr *) &from, sizeof(from));
        }
    }
         
    return 0;
}
