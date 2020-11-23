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
#include <time.h>
#include <sys/types.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void *thread(void *vargp);
char* itoa(int value, char* result, int base);
int dnslookup(const char* hostname, char* firstIPstr, int maxSize);
void * test(void * vargp);
int hostname_to_ip(char *hostname , char *ip);
int build_server_addr(struct sockaddr_in* serv_addr, char * ip_add);
void send_error(int connfd, char * error_msg);
char* itoa(int value, char* result, int base);
int hash(char *str);
void send_black(int connfd, char * error_msg);
char* ultostr(unsigned long value, char *ptr, int base);
struct Thread_object{
    pthread_mutex_t* file_lock;
    int* connfdp;
    int* timeout;
    time_t start_time;
};

int main(int argc, char **argv) 
{
    int listenfd, timeout, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 
    pthread_mutex_t* file_lock;
    file_lock = malloc(sizeof(pthread_mutex_t));
    //initialize file_lock
    if (pthread_mutex_init(file_lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 

    if (argc != 3) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    time_t start_time;
    start_time = time(NULL);
    port = atoi(argv[1]);
    timeout = atoi(argv[2]);

    listenfd = open_listenfd(port);

    //initialize thread_object
    struct Thread_object thread_object;
    thread_object.file_lock = file_lock;
    thread_object.timeout = &timeout;
    thread_object.start_time = start_time;

    printf("Listening on port %d\n", port);
    while (1) {    
        thread_object.connfdp = (intptr_t*)accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, (void *)&thread_object);
    }//while(1)
}//main

/* thread subroutine */
void * thread(void * vargp) 
{  
    //pointer to struct
    struct Thread_object *thread_object;
    struct timeval sock_timeout;
    sock_timeout.tv_sec = 1;
    sock_timeout.tv_usec = 0;
    //thread_object is pointer to thread_obj
    thread_object = (struct Thread_object*)vargp;
    time_t start_time, transfer_start, thread_start, transfer_time, cache_time;
    start_time = thread_object->start_time;
    thread_start = time(NULL);
    int connfd = (int)thread_object->connfdp;
    int timeout = *(thread_object->timeout);
    pthread_detach(pthread_self());     
    
    setsockopt (connfd, IPPROTO_TCP, SO_RCVTIMEO, (char*) &sock_timeout, sizeof (sock_timeout));
    setsockopt (connfd, IPPROTO_TCP, SO_SNDTIMEO, (char*) &sock_timeout, sizeof (sock_timeout));
    //various string vars needed. All declared with malloc so they go on heap. 
    char* resolved_name = malloc(sizeof(char)*MAXBUF);  
    char* url_raw = malloc(sizeof(char)*MAXBUF);  
    char* save_ptr = malloc(sizeof(char)*MAXBUF); 
    char* request = malloc(sizeof(char)*MAXBUF);
    char* request_type = malloc(sizeof(char)*MAXBUF);
    char* http_packet = malloc(sizeof(char) *MAXBUF);
    char* request_header = malloc(sizeof(char) *MAXBUF);
    char* host_name = malloc(sizeof(char)*MAXBUF);
    char* http_response = malloc(sizeof(char)*MAXBUF);
    char* ip = malloc(sizeof(char)*MAXBUF);
    char* page = malloc(sizeof(char)*MAXBUF);
    char* file_ext = malloc(sizeof(char)*MAXBUF);
    char* error_msg = malloc(sizeof(char)*MAXBUF);
    char* hash_string = malloc(sizeof(char)*MAXBUF);
    char* black_list_string = malloc(sizeof(char)*MAXBUF);
    char* is_black_listed = malloc(sizeof(char)*MAXBUF);

    FILE* fp;
    FILE* page_cache;
    FILE* black_list;
    int n;
    int* port;
    int succ_parsing;
    n = read(connfd, request, MAXLINE);

    if(n < 0){
        printf("Bad connection\n");
        pthread_exit(pthread_self);
        close(connfd);
        return NULL;
    }

    sscanf(request, "%[^\n]", request_header);
    sscanf(request, "%s", request_type);

    memset(http_packet, 0, strlen(http_packet)); 
    strcpy(http_packet,request);
   
    //nested if to check if GET request 
    if(strcmp(request_type, "GET") == 0){              
        
        //got touse twice to get second element seperated by space
        strtok_r(request," ", &save_ptr); 
        url_raw = strtok_r(NULL," ", &save_ptr);

        //https://stackoverflow.com/questions/726122/best-ways-of-parsing-a-url-using-c
        //if (sscanf(url_raw, "http://%99[^:]:%i/%199[^\n]", ip, &port, page) == 3) { succ_parsing = 1;}
        if (sscanf(url_raw, "http://%99[^/]/%199[^\n]", ip, page) == 2) { succ_parsing = 1;}
        else if (sscanf(url_raw, "http://%99[^:]:%i[^\n]", ip, &port) == 2) { succ_parsing = 1;}
        else if (sscanf(url_raw, "http://%99[^/]", ip) == 1) { succ_parsing = 1;}
        else{ printf("Error parsing URL\n"); return NULL;}

        char s;
        int j =0;

        pthread_mutex_lock(thread_object->file_lock);

        black_list = fopen("blacklist", "r");

        if(black_list == NULL){
            printf("File not found!\n");
            return NULL;
        }
        s = fgetc(black_list);
        while(s != EOF){
            black_list_string[j]=s;
            s = fgetc(black_list);
            j++;
        }
        is_black_listed = strstr(black_list_string, ip);
        if(is_black_listed != NULL){
            error_msg = "URL is black listed\n";
            send_black(connfd, error_msg);
            close(connfd);
            return NULL;
        }
        pthread_mutex_unlock(thread_object->file_lock);
        if(page[0] == '\0'){
            strcpy(page, ip);
        }
        else{
            file_ext = strtok_r(page, ".", save_ptr);
            file_ext = strtok_r(NULL, ".", save_ptr);
        }        

       /* if(strcmp("http://netsys.cs.colorado.edu/favicon.ico", url_raw)==0){
                   //annoygin request
                    error_msg= "not necessary";
                    send_error(connfd, error_msg);
                    close(connfd);
                    return NULL;

        }*/

        //---- ---CRITICAL SECTION------ lock is good in if and else
        pthread_mutex_lock(thread_object->file_lock);
        fp = fopen(ip, "r");
        
        //if host_name is not saved as a file, we have to do dns lookup
        if( fp == NULL){
            printf("File not found\n");

            int dns_ret = hostname_to_ip(ip, resolved_name);

            if(dns_ret == 0 ){
                printf("Saving address to file\n");
            
                fp = fopen(ip, "w");

                fputs(resolved_name, fp);
                fputs("\n", fp);

                fclose(fp);
                pthread_mutex_unlock(thread_object->file_lock);
            //---------------critical section --------------------
            }//dns_ret if
        }//do dns lookup if
        //file exists
        else{
                fscanf(fp, "%s", resolved_name);
                fclose(fp);
                pthread_mutex_unlock(thread_object->file_lock);
                //---------------critical section --------------------
            }
            //DONE WITH DNS CACHE
            
            cache_time = time(NULL);
            if((cache_time - start_time) > timeout ){

                //need hash to create file for writing still
                int hash_val;
                hash_val = hash(url_raw);
                itoa(hash_val, hash_string, 10);
            }
            else {
                //PAGE CACHE 
                int hash_val;
                hash_val = hash(url_raw);
                itoa(hash_val, hash_string, 10);


                printf("url raw is %s\n",url_raw );
                printf("hash sting is %s\n",hash_string);     

                //---- BEGIN---CRITICAL SECTION------ lock is good in if and else
                pthread_mutex_lock(thread_object->file_lock);
                page_cache = fopen(hash_string, "r");

                if(page_cache == NULL){
                    //have to go to server
                    printf("Page not cached\n");
                    //fclose(page_cache);
                    pthread_mutex_unlock(thread_object->file_lock);
                }
                else{
                    //cached file exists
                    printf("Page is cached\n");
                    //check if it is image
                    if(strcmp(file_ext,"png") == 0 || 
                        strcmp(file_ext,"gif") == 0 || 
                        strcmp(file_ext,"jpg") == 0 ||
                        strcmp(file_ext,"ico") == 0
                        )
                    {

                        //do nothing if it is image
                    }
                    else{
                    //write contents of file to client 
                    char c;
                    int i =0;
                    printf("Writing client from file\n");
                    c = fgetc(page_cache);
                    while(c != EOF){
                        http_response[i]=c;
                        c = fgetc(page_cache);
                        i++;
                    }     
                    write(connfd, http_response, strlen(http_response));
                    fclose(page_cache);
                    printf("Succesful cache transfer\n");            
                    pthread_mutex_unlock(thread_object->file_lock);
                    close(connfd);
                    pthread_exit(pthread_self);
                    return NULL;
                    }
                }
                //---------------END critical section --------------------
            }//outer else

        //DONE WITH page cache

        
        struct sockaddr_in* serv_addr = malloc(sizeof(struct sockaddr_in));
        int sock;
        int check_addr; 

        //takes pointer to sockaddr_in and ip_addr as string
        check_addr = build_server_addr(serv_addr, resolved_name);

        if(check_addr < 0){
            printf("Error building address\n");
            error_msg = "Error building address\n";
            send_error(connfd, error_msg);
            pthread_exit(pthread_self);
            return NULL;
        }
        else{
            if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
            { 
                printf("\n Socket creation error \n"); 
                error_msg = "Socket creation error \n";
                send_error(connfd, error_msg);
                pthread_exit(pthread_self);
                return NULL; 
            } 
            if(connect(sock, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in)) < 0) 
                { 
                    printf("\nConnection Failed \n"); 
                    pthread_exit(pthread_self);
                    error_msg = "Connection failed\n";
                    send_error(connfd, error_msg);
                    return NULL; 
                }
            int n;



            setsockopt (sock, IPPROTO_TCP, SO_RCVTIMEO, (char*) &sock_timeout, sizeof (sock_timeout));
            setsockopt (sock, IPPROTO_TCP, SO_SNDTIMEO, (char*) &sock_timeout, sizeof (sock_timeout));
            //write initial get request to server
            n = write(sock, http_packet, strlen(http_packet));
            if(n<0){
                printf("Error writing\n");
                close(sock);
                close(connfd);
                pthread_exit(pthread_self);
                return NULL;
            }
            printf("wrote %d bytes to server\n", n);

            //---- ---CRITICAL SECTION----------------------
            pthread_mutex_lock(thread_object->file_lock);

            page_cache = fopen(hash_string, "a");

            if(page_cache == NULL){
                printf("Error creating cache file");
                pthread_mutex_unlock(thread_object->file_lock);
                pthread_exit(pthread_self);
                return NULL;
            }

            
            printf("Transferring data\n");
            transfer_start = time(NULL);
            transfer_time = transfer_start - thread_start;

            //https://stackoverflow.com/questions/4181784/how-to-set-socket-
            //timeout-in-c-when-making-multiple-connections#:~:text=You%20can
            //%20use%20the%20SO_RCVTIMEO,tv_sec%20%3D%2010%3B%20timeout.

            while(transfer_time < 5){
                int bytes_read;
                int m;
                
                printf("transfer time %ld\n",transfer_time );

                memset(http_response, 0, MAXBUF); 
                bytes_read = read(sock, http_response, MAXBUF);
                if(bytes_read < 0){
                    printf("Error reading from network socket.\n");
                    close(sock);
                    close(connfd);
                    pthread_exit(pthread_self);
                    return NULL;
                }
                printf("Read %d bytes from server\n", bytes_read);

                if(transfer_time > 10){
                    printf("Exiting program\n");
                    fclose(page_cache);
                    pthread_mutex_unlock(thread_object->file_lock);
                    break;
                }

                http_response[bytes_read] = '\0';
                //save results to file before sending to client 
                fputs(http_response, page_cache);

                if(bytes_read> 0){

                    m = write(connfd, http_response, bytes_read);
                    if(m < 0){
                        printf("Error writing back to client\n");
                        close(sock);
                        close(connfd);
                        pthread_exit(pthread_self);
                        return NULL;
                    }
                    printf("wrote %d bytes back to client\n",m);
                }
                transfer_start = time(NULL);
                transfer_time = transfer_start - thread_start;

                if(bytes_read == 0){                
                    //exit loop
                    printf("Exiting program\n");
                    fclose(page_cache);
                    pthread_mutex_unlock(thread_object->file_lock);
                    break;
                }
            }//forever while
        free(resolved_name);
        free(request);
        free(http_packet);
        free(host_name);
        free(http_response);
        close(connfd);
        pthread_exit(pthread_self);
        return NULL;        
    }//if address was built right else

    }//nested if

    //not a GET request, bye
    else{
        //printf("Proxy only handles GET. Sorry\n");
        error_msg = "Bad request";
        send_error(connfd, error_msg);
        free(resolved_name);
        free(request);
        free(http_packet);
        free(host_name);
        free(http_response);
        close(connfd);
        pthread_exit(pthread_self);
        return NULL;
    }//terminating else
  }//thread  

/*


 

HELPER FUNCTIONS BELOW  





*/
int build_server_addr(struct sockaddr_in* serv_addr, char * ip_add){
    printf("Building address\n");
    serv_addr->sin_family = AF_INET; //ipV4
    serv_addr->sin_port = htons(80);

    //https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
    //converts IP to correct format
    if(inet_aton(ip_add, (struct in_addr* )&serv_addr->sin_addr.s_addr) ==0){
        return -1;
    }
    printf("Address built successfully\n");
    return 1;
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

//https://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
int hostname_to_ip(char *hostname , char *ip)
{
    //int sockfd;  
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

void send_error(int connfd, char * error_msg)
{

    char httpmsg[]="HTTP/1.1 400 Bad Request\r\nContent-Type:text/plain\r\nContent-Length:";
    char after_content_length[]="\r\n\r\n";
    char content_length[MAXLINE];
    
    //convert file size int to string
    itoa(strlen(error_msg), content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);    

    //add file contents to http header
    strcat(httpmsg, error_msg);
    //printf("server returning a http message with the following content.\n%s\n",httpmsg);
    write(connfd, httpmsg, strlen(httpmsg));
    
}

char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

//https://stackoverflow.com/questions/7666509/hash-function-for-string
int hash(char *str)
{
    int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

char* ultostr(unsigned long value, char *ptr, int base)
{
  unsigned long t = 0, res = 0;
  unsigned long tmp = value;
  int count = 0;

  if (NULL == ptr)
  {
    return NULL;
  }

  if (tmp == 0)
  {
    count++;
  }

  while(tmp > 0)
  {
    tmp = tmp/base;
    count++;
  }

  ptr += count;

  *ptr = '\0';

  do
  {
    res = value - base * (t = value / base);
    if (res < 10)
    {
      * -- ptr = '0' + res;
    }
    else if ((res >= 10) && (res < 16))
    {
        * --ptr = 'A' - 10 + res;
    }
  } while ((value = t) != 0);

  return(ptr);
}

void send_black(int connfd, char * error_msg)
{


    char httpmsg[]="HTTP/1.1 403 Forbidden\r\nContent-Type:text/plain\r\nContent-Length:";
    char after_content_length[]="\r\n\r\n";
    char content_length[MAXLINE];
    
    //convert file size int to string
    itoa(strlen(error_msg), content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);    

    //add file contents to http header
    strcat(httpmsg, error_msg);
    //printf("server returning a http message with the following content.\n%s\n",httpmsg);
    write(connfd, httpmsg, strlen(httpmsg));
    
}