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
void * test(void * vargp);
int hostname_to_ip(char *hostname , char *ip);

struct Thread_object{
    FILE* cached;
    pthread_mutex_t* file_lock;
    int* connfdp;
};

#define UTIL_FAILURE -1
#define UTIL_SUCCESS 0

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 
    pthread_mutex_t* file_lock;
    FILE *cached;
    file_lock = malloc(sizeof(pthread_mutex_t));

   /* cached = fopen("cached_domains.txt", "w");
    if(cached == NULL){
        printf("File not found!\n");
    }*/

    //initialize file_lock
    if (pthread_mutex_init(file_lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);

    //initialize thread_object
    struct Thread_object thread_object;
    thread_object.file_lock = file_lock;
    thread_object.cached = cached;
   /* thread_object.cached = fopen("cached_domains.txt", "wa");
    fseek(thread_object.cached, 0, SEEK_END);
    fputs("Hey testjfkljlking", thread_object.cached);
    fclose(thread_object.cached);*/
    printf("Listening on port %d\n", port);
    while (1) {    
    connfdp = malloc(sizeof(int));
    thread_object.connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
    pthread_create(&tid, NULL, thread, (void *)&thread_object);
    //pthread_create(&tid, NULL, test, (void *)&thread_object);
    }
}






   /* 
    char * resolved_name = malloc(sizeof(char)*MAXBUF);
    hostname_to_ip("google.com", resolved_name);
    printf("%s\n",resolved_name );

   while (1) {    
    //prepare thread object to be sent to thread
    struct Thread_object thread_obj;
    thread_obj.ip_addr = (char *)malloc(sizeof(char) *100);
    thread_obj.connfdp = malloc(sizeof(int));
    thread_obj.connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);

    pthread_create(&tid, NULL, thread, (void *)&thread_obj);
    }*/


void * test(void * vargp) 
{  

    struct Thread_object *thread_object;
    thread_object = vargp;

    int connfd = thread_object->connfdp;

    
    pthread_detach(pthread_self()); 
    size_t n;

    char* resolved_name = malloc(sizeof(char)*100);
    char* domain_name =malloc(sizeof(char)*100);
    strcpy(domain_name, "google.com");
    hostname_to_ip(domain_name, resolved_name);
    printf("%s\n",resolved_name);



}
/* thread routine */
void * thread(void * vargp) 
{  
    struct Thread_object *thread_object;
    thread_object = vargp;

    int connfd = thread_object->connfdp;
    pthread_detach(pthread_self()); 
    size_t n;

    //various string vars needed. All declared with malloc so they go on heap. 
    char* ip_add = malloc(sizeof(char)*MAXBUF);
    char* resolved_name = malloc(sizeof(char)*MAXBUF);  
    char* request = malloc(sizeof(char)*MAXBUF);
    char* request_type = malloc(sizeof(char)*MAXBUF);
    char* http_packet = malloc(sizeof(char) *MAXBUF);
    char* request_header_token = malloc(sizeof(char) *MAXBUF);
    char* domain_name = malloc(sizeof(char)*MAXBUF);
    char* host_name = malloc(sizeof(char)*MAXBUF);

    n = read(connfd, request, MAXLINE);

    if(n < 0){
        printf("Bad connection\n");
    }

    //copy packet info before parsing
    strcpy(http_packet,request);
    
    //gets first element of http packet
    request_type = strtok(request, " ");      

    //nested if to check if GET request 
    if(strcmp(request_type, "GET") == 0){
        
        //gets next element in http request
        domain_name = strtok(NULL, " ");

        host_name = strtok(domain_name, "http://");

        //-------CRITICAL SECTION------ lock is good in if and else
        pthread_mutex_lock(thread_object->file_lock);
        thread_object->cached = fopen(host_name, "r");
        
        //if host_name is not saved as a file, we have to do dns lookup
        if( thread_object->cached == NULL){
            printf("File not found\n");

            int dns_ret = hostname_to_ip(host_name, resolved_name);
            printf("%s\n",resolved_name);
            if(dns_ret == 0 ){
                printf("Saving address to file\n");
            
                thread_object->cached = fopen(host_name, "w");

                fputs(resolved_name, thread_object->cached);
                fputs("\n", thread_object->cached);

                fclose(thread_object->cached);
                pthread_mutex_unlock(thread_object->file_lock);
            //---------------critical section --------------------
            }//dns_ret if
        }//do dns lookup if
        //file exists
        else{
            fscanf(thread_object->cached, "%s", resolved_name);
            printf("resolved from file is %s\n", resolved_name);
            fclose(thread_object->cached);
            pthread_mutex_unlock(thread_object->file_lock);

        }


        //DONE WITH DNS CACHE
    



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

int create_send_socket(char* ip_add)
{

    struct sockaddr_in serv_addr;
    int sock = 0, valread;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    }//if
    

}//create_send_socket



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