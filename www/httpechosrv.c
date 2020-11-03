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

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);

void get_html(int connfd, char *requested_file, int file_size);
void get_image(int connfd, FILE *requested_file, char *extension);
void get_css(int connfd, FILE *requested_file, char *extension);
void get_js(int connfd, FILE *requested_file, char *extension);
void get_text(int connfd, char *requested_file, int file_size);
void get_error(int connfd, char * error_msg);

char* itoa(int value, char* result, int base);

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

    while (1) {    
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
  /*  char cwd[MAXBUF];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("Current working dir: %s\n", cwd);
   }*/

    int connfd = *((int *)vargp);
    free(vargp);
    pthread_detach(pthread_self()); 
    size_t n;
  
    char request[MAXLINE];


    //read from browser
    char *request_header;
    request_header = (char *)malloc(sizeof(char) *100);
    n = read(connfd, request, MAXLINE);

    //get first line of http request
    //gives string <request type> </wherever> <HTTP/1.1>
    
    request_header = strtok(request, "\n");  
    char *request_header_token;
    request_header_token = (char *)malloc(sizeof(char) *100);
    request_header_token = strtok(request_header," ");

    //parse string further
    char *get_request;
    get_request = (char *)malloc(sizeof(char) *100);
    get_request = strtok(NULL, " ");

    //we now have user file request
    
    //checks if address has period, if not not valid
    if(strstr(get_request, ".") != NULL || strcmp(get_request, "/")==0){

    if(strcmp(get_request,"/socketcluster/")==0){
        return NULL;
    }

    //get extension of file
    char *extension;
    extension = (char *)malloc(sizeof(char) *100);
    char *parse;
    parse = (char *)malloc(sizeof(char) *100);
    char get_request_copy[MAXBUF];
    strcpy(get_request_copy, get_request);

    //get first part of request
    parse = strtok(get_request_copy, ".");

    
    if (strcmp(get_request,"/")==0){
        extension = "html" ;     
            }//if
        else{    
        //get extension
        extension = strtok(NULL, ".");
    }//else for extension


    //handle further parsing when needed
    if(
        strcmp(extension,"mousewheel-3") == 0 || 
        strcmp(extension,"fancybox-1") == 0 || 
        strcmp(extension,"4") == 0 ||
        strcmp(extension,"easing-1") == 0
        )
    {
        if(strcmp(get_request,"jquery-fancybox-1.3.4.css") == 0){
            strcpy(extension, "css");
        }
        else if(strcmp(get_request,"jquery.fancybox-1.3.4.pack.js")){            
            strcpy(extension, "js");
        }
        else{
            strcpy(extension, "js");
        }
    }//further parsing

printf("%s has thread id %lu and extension %s \n", get_request, pthread_self, extension);    

    //load index page
    if(strcmp(get_request, "/")==0){
        FILE *fp;
        fp = fopen("index.html", "r");
        //copy file contents to string file_contents
        char file_contents[MAXLINE];
        int i =0;
        char c;
        int file_size;
        c = fgetc(fp);
        while(c != EOF){
            file_contents[i] = c;
            i++;
            c =fgetc(fp);
        }    
        file_size= strlen(file_contents);
        get_html(connfd, file_contents, file_size);
    }//if index

    //handle images
    else if(
        strcmp(extension,"png") == 0 || 
        strcmp(extension,"gif") == 0 || 
        strcmp(extension,"jpg") == 0 ||
        strcmp(extension,"ico") == 0
        )
    {
        //adds . to files so they can be accessed
        //explored trying to change cd to root but seems like not necessary   
        printf("handling image\n"); 
        char period[MAXBUF];
        strcpy(period, ".");
        char get_request_cur_dir[MAXBUF];
        strcat(period, get_request);
        strcpy(get_request_cur_dir, period);


        printf("%s \n", get_request_cur_dir);
        //open file in binary mode
        FILE *fp;
        fp = fopen(get_request_cur_dir, "rb");
        if(fp == NULL){
            printf("File not found!\n");
            char error_msg[]="File not found!\n";
            get_error(connfd, error_msg);
        }

        get_image(connfd, fp, extension);
        //printf("%s\n",file_contents);*/
        //get_image(connfd, fp);
    } //image elif

    //handle html and txt
    else if (
        strcmp(extension, "html") == 0 ||
        strcmp(extension, "html~") == 0 ||
        strcmp(extension, "txt") == 0
        )
    {

        //adds . to files so they can be accessed
        //explored trying to change cd to root but seems like not necessary
        char period[MAXBUF];
        strcpy(period, ".");
        char get_request_cur_dir[MAXBUF];
        strcat(period, get_request);
        strcpy(get_request_cur_dir, period);
        FILE *fp;
        fp = fopen(get_request_cur_dir, "r");
        if(fp == NULL){
            printf("File not found!\n");
            char error_msg[]="File not found!\n";
            get_error(connfd, error_msg);
        }

        //copy file contents to string file_contents
        char file_contents[MAXLINE];
        int i =0;
        char c;
        c = fgetc(fp);
        while(c != EOF){
            file_contents[i] = c;
            i++;
            c =fgetc(fp);
        }
        int file_size;
        file_size= strlen(file_contents);
        
        if(strcmp(extension,"txt")==0){
            get_text(connfd, file_contents, file_size);
        }
        else{
            get_html(connfd, file_contents, file_size);
        }        
   }//else if html

//handle js and css
    else if(
        strcmp(extension,"js") == 0 || 
        strcmp(extension,"css") == 0
        )
    {

        //adds . to files so they can be accessed
        //explored trying to change cd to root but seems like not necessary
        char period[MAXBUF];
        strcpy(period, ".");
        char get_request_cur_dir[MAXBUF];
        strcat(period, get_request);
        strcpy(get_request_cur_dir, period);
        
        FILE *fp;
        fp = fopen(get_request_cur_dir, "r");
        
        if(fp == NULL){
            printf("File not found!\n");
            return NULL;
        }

       
        if(strcmp(extension,"js") == 0){        
            get_js(connfd, fp, extension);
        }
        else{
            get_css(connfd, fp,extension);
        }

    }//js and css elif
    else if(extension == NULL){
        char error_msg[]="File extension was NULL";
        get_error(connfd, error_msg);
    }
    else{
        char error_msg[]="File extension was not recognized";
        get_error(connfd, error_msg);
    }    
    close(connfd);
    return NULL;

}//nested if
else{
    char error_msg[]="File was not recognized";
    get_error(connfd, error_msg);
}

}//thread

/*


 

HELPER FUNCTIONS BELOW

 


 */
void get_html(int connfd, char *requested_file, int file_size) 
{    
    char buf[MAXLINE]; 
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:";
    char after_content_length[]="\r\n\r\n";
    char content_length[MAXLINE];
    
    //convert file size int to string
    itoa(file_size, content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);   

    //add file contents to http header
    strcat(httpmsg, requested_file);
    //printf("server returning a http message with the following content.\n%s\n",httpmsg);
    write(connfd, httpmsg,strlen(httpmsg));
    
}
void get_text(int connfd, char *requested_file, int file_size) 
{    
    char buf[MAXLINE]; 
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/plain\r\nContent-Length:";
    char after_content_length[]="\r\n\r\n";
    char content_length[MAXLINE];
    
    //convert file size int to string
    itoa(file_size, content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);
    

    //add file contents to http header
    strcat(httpmsg, requested_file);
    //printf("server returning a http message with the following content.\n%s\n",httpmsg);
    write(connfd, httpmsg,strlen(httpmsg));
    
}

//source: https://stackoverflow.com/questions/15445207/sending-image-jpeg-through-socket-in-c-linux
void get_image(int connfd, FILE *requested_file, char *extension) 
{   
    if(requested_file == NULL){

    }
    else{
    char *sendbuf;
    int file_size;
    char *binary_data;
    char buf[MAXLINE]; 
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:image/";
    char after_extension[] = "\r\nContent-Length:";

    //add type of image
    strcat(httpmsg, extension);
    strcat(httpmsg, after_extension);
    
    char after_content_length[]="\r\n\r";
    char content_length[MAXLINE];
    
    fseek (requested_file, 0, SEEK_END);

    file_size = ftell(requested_file);
    rewind(requested_file);         
    sendbuf = (char*) malloc (sizeof(char)*file_size);
    size_t result = fread(sendbuf, 1, file_size, requested_file);

    //convert file size int to string
    itoa(file_size, content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);
    strcat (httpmsg, "Connection: keep-alive\r\n\r\n");

    //send http header
    write(connfd, httpmsg,strlen(httpmsg));

    int buf_index = 0;
    int packets = file_size / MAXBUF;
    
    //send binary data
    write(connfd, sendbuf, file_size);
    }//else
}
void get_js(int connfd, FILE *requested_file, char *extension) 
{   
    if(requested_file == NULL){

    }
    else{
    char *sendbuf;
    int file_size;
    char *binary_data;
    char buf[MAXLINE]; 
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:application/";
    char after_extension[] = "\r\nContent-Length:";

    //add js
    strcat(httpmsg, extension);
    strcat(httpmsg, after_extension);
    
    char after_content_length[]="\r\n\r";
    char content_length[MAXLINE];
    
    fseek (requested_file, 0, SEEK_END);

    file_size = ftell(requested_file);
    rewind(requested_file);         
    sendbuf = (char*) malloc (sizeof(char)*file_size);
    size_t result = fread(sendbuf, 1, file_size, requested_file);

    //convert file size int to string
    itoa(file_size, content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);
    strcat (httpmsg, "Connection: keep-alive\r\n\r\n");

    //send http header
    write(connfd, httpmsg,strlen(httpmsg));

    int buf_index = 0;
    int packets = file_size / MAXBUF;
    
    //send data
    write(connfd, sendbuf, file_size);    
    }//else
}//get js

void get_css(int connfd, FILE *requested_file, char *extension) 
{  
    if(requested_file == NULL){

    }
    else{ 
    char *sendbuf;
    int file_size;
    char *binary_data;
    char buf[MAXLINE]; 
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/";
    char after_extension[] = "\r\nContent-Length:";

    //add css
    strcat(httpmsg, extension);
    strcat(httpmsg, after_extension);
    
    char after_content_length[]="\r\n\r";
    char content_length[MAXLINE];
    
    fseek (requested_file, 0, SEEK_END);

    file_size = ftell(requested_file);
    rewind(requested_file);         
    sendbuf = (char*) malloc (sizeof(char)*file_size);
    size_t result = fread(sendbuf, 1, file_size, requested_file);

    //convert file size int to string
    itoa(file_size, content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);
    strcat (httpmsg, "Connection: keep-alive\r\n\r\n");

    //send http header
    write(connfd, httpmsg,strlen(httpmsg));

    int buf_index = 0;
    int packets = file_size / MAXBUF;
    
    //send binary data
    write(connfd, sendbuf, file_size);    
    }//else
}//get css
void get_error(int connfd, char * error_msg)
{

    char buf[MAXLINE]; 
    char httpmsg[]="HTTP/1.1 500 Internal Server Error\r\nContent-Type:text/plain\r\nContent-Length:";
    char after_content_length[]="\r\n\r\n";
    char content_length[MAXLINE];
    
    //convert file size int to string
    itoa(strlen(error_msg), content_length, 10);
    
    strcat(httpmsg, content_length);
    strcat(httpmsg, after_content_length);    

    //add file contents to http header
    strcat(httpmsg, error_msg);
    //printf("server returning a http message with the following content.\n%s\n",httpmsg);
    write(connfd, httpmsg,strlen(httpmsg));
    
}
/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
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

//function to convert into to string
//source:https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c/23840699#:~:text=You%20can%20use%20itoa(),to%20convert%20any%20value%20beforehand.
/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
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