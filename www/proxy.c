#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void *thread(void *vargp);
char* itoa(int value, char* result, int base);
int dnslookup(const char* hostname, char* firstIPstr, int maxSize);

struct Thread_object{
    int* connfdp;
    char* ip_addr;
};

#define UTIL_FAILURE -1
#define UTIL_SUCCESS 0

char global_string[100];
char global_google[100] = "google.com";
int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    printf("listening on port %d\n", port);
    int dnslookup(global_google,global_string,1000)
    printf("%s\n",global_string );

  /*  while (1) {    
    //prepare thread object to be sent to thread
    struct Thread_object thread_obj;
    thread_obj.ip_addr = (char *)malloc(sizeof(char) *100);
    thread_obj.connfdp = malloc(sizeof(int));
    thread_obj.connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);

    pthread_create(&tid, NULL, thread, (void *)&thread_obj);
    }*/
    printf("global_google outside thread address %p\n", &global_string );
    while (1) {    
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  

    struct Thread_object *thread_object;
    thread_object = vargp;


    int connfd = *((int *)vargp);
    free(vargp);
    printf("global_google in thread address %p\n", &global_string );
    //int connfd = thread_object->connfdp;

    pthread_detach(pthread_self()); 
    size_t n;
  
    char request[MAXLINE];


    //read from browser
    char *request_header;
    request_header = (char *)malloc(sizeof(char) *100);
    n = read(connfd, request, MAXLINE);

    if(n< 0){
        printf("Bad connection\n");
    }

    //copy packet info before parsing
    char *http_packet;
    http_packet = (char *)malloc(sizeof(char) *100);
    strcpy(http_packet, request);
    
    //get first line of http request
    //gives string <request type> </wherever> <HTTP/1.1>    
    request_header = strtok(request, "\n");  
    char *request_header_token;
    request_header_token = (char *)malloc(sizeof(char) *100);
    request_header_token = strtok(request_header," ");

    //nested if to check if GET request 
    if(strcmp(request_header_token, "GET") == 0){

        printf("http packet is \n%s\n",http_packet );

        //parse string further
        char *domain_name;
        domain_name = (char *)malloc(sizeof(char) *100);
        domain_name = strtok(NULL, " ");

        getaddrinfo(domain_name, NULL, NULL, &headresult);

        printf("%s\n",domain_name );

        return NULL;
    }//nested if

    //not a GET request, bye
    else{
        printf("Proxy only handles GET. Sorry\n");
        return NULL;
    }//terminating else
  }//thread  

/*


 

HELPER FUNCTIONS BELOW

 


 */

int send_packet_to_internet(char* http_packet, char* domain_name){

}



int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */


/*int socket_connect(char *host, in_port_t port){
    struct hostent *hp;
    struct sockaddr_in addr;
    int on = 1, sock;     

    if((hp = gethostbyname(host)) == NULL){
        herror("gethostbyname");
        exit(1);
    }
    copy(hp->h_addr, &addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(sock == -1){
        perror("setsockopt");
        exit(1);
    }
    
    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        exit(1);

    }
    return sock;
}*/

//https://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
int dnslookup(const char* hostname, char* firstIPstr, int maxSize){

    /* Local vars */
    struct addrinfo* headresult = NULL;
    struct addrinfo* result = NULL;
    struct sockaddr_in* ipv4sock = NULL;
    struct in_addr* ipv4addr = NULL;
    char ipv4str[INET_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];
    int addrError = 0;

    /* DEBUG: Print Hostname*/
#ifdef UTIL_DEBUG
    fprintf(stderr, "%s\n", hostname);
#endif
   
    /* Lookup Hostname */
    addrError = getaddrinfo(hostname, NULL, NULL, &headresult);
    if(addrError){
    fprintf(stderr, "Error looking up Address: %s\n",
        gai_strerror(addrError));
    return UTIL_FAILURE;
    }
    /* Loop Through result Linked List */
    for(result=headresult; result != NULL; result = result->ai_next){
    /* Extract IP Address and Convert to String */
    if(result->ai_addr->sa_family == AF_INET){
        /* IPv4 Address Handling */
        ipv4sock = (struct sockaddr_in*)(result->ai_addr);
        ipv4addr = &(ipv4sock->sin_addr);
        if(!inet_ntop(result->ai_family, ipv4addr,
              ipv4str, sizeof(ipv4str))){
        perror("Error Converting IP to String");
        return UTIL_FAILURE;
        }
#ifdef UTIL_DEBUG
        fprintf(stdout, "%s\n", ipv4str);
#endif
        strncpy(ipstr, ipv4str, sizeof(ipstr));
        ipstr[sizeof(ipstr)-1] = '\0';
    }
    else if(result->ai_addr->sa_family == AF_INET6){
        /* IPv6 Handling */
#ifdef UTIL_DEBUG
        fprintf(stdout, "IPv6 Address: Not Handled\n");
#endif
        strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
        ipstr[sizeof(ipstr)-1] = '\0';
    }
    else{
        /* Unhandlded Protocol Handling */
#ifdef UTIL_DEBUG
        fprintf(stdout, "Unknown Protocol: Not Handled\n");
#endif
        strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
        ipstr[sizeof(ipstr)-1] = '\0';
    }
    /* Save First IP Address */
    if(result==headresult){
        strncpy(firstIPstr, ipstr, maxSize);
        firstIPstr[maxSize-1] = '\0';
    }
    }

    /* Cleanup */
    freeaddrinfo(headresult);

    return UTIL_SUCCESS;
}//dns lookup