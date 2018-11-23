#include <stdio.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#define PORT 3940
#define NUM_FORKS 10 
   
int main(int argc, char const *argv[]) 
{ 
    struct sockaddr_in address; 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    pid_t pid;

    for(int i = 0; i < NUM_FORKS; i++){
        pid = fork();
        if(pid == 0) break;
    }
    if(pid < 0){
	printf("Fork falló\n");
        return 1;
    }
    else if(pid == 0){
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
	    printf("\n Socket creation error \n"); 
	    return -1; 
        } 
        memset(&serv_addr, '0', sizeof(serv_addr)); 
        serv_addr.sin_family = AF_INET; 
        serv_addr.sin_port = htons(PORT);
        // Convert IPv4 and IPv6 addresses from text to binary form 
        if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0){ 
	    printf("\nInvalid address/ Address not supported \n"); 
	    return -1; 
        } 
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ 
	    printf("\nConnection Failed \n"); 
	    return -1; 
        } 
        send(sock , "GET /despacito.mp4 HTTP/1.1\n\n" , 30 , 0 );
        return 0; 
    }
    else{
        waitpid(pid, NULL, 0);
    }
    return 0;    
} 
