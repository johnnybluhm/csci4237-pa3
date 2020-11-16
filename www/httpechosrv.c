/* 
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */

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

//from PA3 operating systems
#define UTIL_FAILURE -1
#define UTIL_SUCCESS 0

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void *thread(void *vargp);
char* itoa(int value, char* result, int base);
int hostname_to_ip(char* hostname , char* ip);

struct Thread_object{
    int* connfdp;
    char* ip_addr;
};

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
    hostname_to_ip(global_google, global_string);
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

        printf("%s\n",domain_name );
        
        
        //resolved_domain = (char *)malloc(sizeof(char) *100); 
        hostname_to_ip(global_google, global_string);
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
int hostname_to_ip(char *hostname , char *ip)
{
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        h = (struct sockaddr_in *) p->ai_addr;
        strcpy(ip , inet_ntoa( h->sin_addr ) );
    }
    
    freeaddrinfo(servinfo); // all done with this structure
    return 0;
}