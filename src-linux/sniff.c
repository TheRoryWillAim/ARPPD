#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <string.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

char gtwy[128];
char gateway_ip[128];

void rearpGateway(char* gateway_mac, char* gateway_ip){
  //arp -s 10.0.0.2 00:0c:29:c0:94:bf
  FILE *fp;
  char path[1035];
  int exists = 0;
  char command[1000];
  snprintf(command, sizeof(command), "arp -s %s %s", gateway_ip, gateway_mac);
  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to block packets\n" );
    exit(1);
  }
  printf("%sREARPED %s TO %s%s\n", KGRN, gateway_ip, gateway_mac, KWHT);
}

void blockARPPackets(char* mac_address){
  FILE *fp;
  char path[1035];
  int exists = 0;
  char command[1000];
  snprintf(command, sizeof(command), "arptables -A INPUT --source-mac %s -j DROP", mac_address);
  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to block packets\n" );
    exit(1);
  }
  printf("%sBLOCKING PACKETS FROM %s%s\n", KGRN, mac_address, KWHT);
}

void my_callback(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char*
    packet)
{
    int i=0;
    static int count=0;

    char p_source[128] = "";
    char arp_source[128] = "";
    char arp_ip_source[128] = "";

    // printf("Packet Count: %d\n", ++count);    /* Number of Packets */
    // printf("Recieved Packet Size: %d\n", pkthdr->len);    /* Length of header */
    // printf("Payload:\n");                     /* And now the data */
    for(i=0;i<pkthdr->len;i++) {

        if(i >= 6 && i<= 11){
          //Daca el vrea sa si schimbe ip ul lui si exista deja in arp, blocheaza!
          char j[8];
          snprintf(j, sizeof(j), "%02x", packet[i]);
          strcat(p_source, j);
          if (i != 11)
          strcat(p_source, ":");
        }



        if(i >= 22 && i<= 27){
          //Daca el vrea sa si schimbe ip ul lui si exista deja in arp, blocheaza!
          char f[8];
          snprintf(f, sizeof(f), "%02x", packet[i]);
          strcat(arp_source, f);
          if(i != 27)
          strcat(arp_source, ":");
        }

        if(i >= 28 && i<= 31){
          //Daca el vrea sa si schimbe ip ul lui si exista deja in arp, blocheaza!
          char h[8];
          snprintf(h, sizeof(h), "%d", packet[i]);
          strcat(arp_ip_source, h);
          if(i != 31)
          strcat(arp_ip_source, ".");
        }
    }
    // printf("Extracted source: %s\n", p_source);
    // printf("Extracted ARP source: %s\n", arp_source);
    // printf("Extracted ARP IP Source: %s\n", arp_ip_source);

      char attacker_ip[128] = "";
            if(strncmp(arp_source, gtwy, 20) != 0){
              //Check if ARP Source IP is GATEWAY
              if(strncmp(arp_ip_source, gateway_ip, 20) == 0){
                //MALICIOUS ARP PACKET!
                printf("\x1B[31m MALICIOUS ARP PACKET DETECTED FROM %s.\x1B[37m\n", p_source);
                blockARPPackets(arp_source);
                rearpGateway(gtwy, gateway_ip);
              }
            }





    }


int sniffARPPackets(char* gateway, char* gateway_ipp)
{

    strcpy(gtwy, gateway);
    strcpy(gateway_ip, gateway_ipp);
    int i;
    char *dev;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr;
    const u_char *packet;
    struct pcap_pkthdr hdr;
    struct ether_header *eptr;    /* net/ethernet.h */
    struct bpf_program fp;        /* hold compiled program */
    bpf_u_int32 maskp;            /* subnet mask */
    bpf_u_int32 netp;             /* ip */

    // if(argc != 2){
    //     fprintf(stdout, "Usage: %s \"expression\"\n"
    //         ,argv[0]);
    //     return 0;
    // }

    /* Now get a device */
    dev = pcap_lookupdev(errbuf);

    if(dev == NULL) {
        fprintf(stderr, "%s\n", errbuf);
        exit(1);
    }
        /* Get the network address and mask */
    pcap_lookupnet(dev, &netp, &maskp, errbuf);

    /* open device for reading in promiscuous mode */
    descr = pcap_open_live(dev, BUFSIZ, 1,-1, errbuf);
    if(descr == NULL) {
        printf("pcap_open_live(): %s\n", errbuf);
        exit(1);
    }

    /* Now we'll compile the filter expression*/
    if(pcap_compile(descr, &fp, "arp", 0, netp) == -1) {
        fprintf(stderr, "Error calling pcap_compile\n");
        exit(1);
    }

    /* set the filter */
    if(pcap_setfilter(descr, &fp) == -1) {
        fprintf(stderr, "Error setting filter\n");
        exit(1);
    }

    /* loop for callback function */
    pcap_loop(descr, -1, my_callback, NULL);
    return 0;
}