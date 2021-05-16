#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<netdb.h>
#include <sys/types.h>

#define BUFMAXLEN 1024
#define PORT_NO 10034

int main(int argc,char*argv[]){	

	if(argv[1] == NULL){
		perror("Error:please enter address");
		exit(1);
	}
	
	struct hostent*hp;
	struct sockaddr_in sa;
	char message[BUFMAXLEN],buf[BUFMAXLEN];
	int vlen;
	
	memset(message,'\0',BUFMAXLEN);
	memset(buf,'\0',BUFMAXLEN);
	
	if((hp = gethostbyname(argv[1])) == NULL){
		fprintf(stderr,"Fail to get host name: %s\n",argv[1]);
		exit(1);
	}
	
	int c_s = socket(AF_INET,SOCK_STREAM,0);
	if(c_s < 0){
		perror("Error:socket()");
		exit(1);
	}
	
	
	sa.sin_family = hp->h_addrtype; // host address type
	sa.sin_port = htons(PORT_NO); // port number
	bzero((char *) &sa.sin_addr, sizeof(sa.sin_addr));
	memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
	

	if(connect(c_s,(struct sockaddr*)&sa,sizeof(struct hostent)) == -1){
		perror("Error:connect()");
		exit(1);
	}
	
	while(0 < read(0,message,BUFMAXLEN)){
		
		if(send(c_s,message,strlen(message),0) == -1){
			perror("Error:send()");
			exit(1);
		}
	
		
		if((vlen = recv(c_s,buf,sizeof(buf),0)) > 0){
			if(strcmp(buf,"end\n") == 0) {
				write(1,buf,vlen);
				exit(0);
			}
			
			else if (strcmp(buf,"read file\n")== 0){
				write(1,buf,vlen);
				while(1){
				memset(message,'\0',BUFMAXLEN);
				memset(buf,'\0',BUFMAXLEN);
				sprintf(message,"OK");
				send(c_s,message,strlen(message),0);
				vlen = recv(c_s,buf,sizeof(buf),0);
				write(1,buf,vlen);
				if(strcmp(buf,"end\n") == 0) {
					exit(0);
				}
				else if(strcmp(buf,"show profiles\n") == 0){
				while(1){
					memset(message,'\0',BUFMAXLEN);
					memset(buf,'\0',BUFMAXLEN);
					sprintf(message,"ACK");
					send(c_s,message,strlen(message),0);
					vlen = recv(c_s,buf,sizeof(buf),0);
					write(1,buf,vlen);
					if(strcmp(buf,"END\n") == 0) break;
					}
				}
					else if(strcmp(buf,"end\n") == 0) {
						exit(0);
					}
				else if(strcmp(buf,"read all text\n") == 0) break;
				}
			}
			
			else if(strcmp(buf,"show profiles\n") == 0){
				write(1,buf,vlen);
				while(1){
				memset(message,'\0',BUFMAXLEN);
				memset(buf,'\0',BUFMAXLEN);
				sprintf(message,"ACK");
				send(c_s,message,strlen(message),0);
				vlen = recv(c_s,buf,sizeof(buf),0);
				write(1,buf,vlen);
				if(strcmp(buf,"END\n") == 0) break;
				}
			}
			
			else write(1,buf,vlen);
		}
	
		else if(vlen < 0){
			perror("Error:recv()");
			break;
		}
		else if(vlen == 0) break;
	
			
	memset(message,'\0',BUFMAXLEN);
	memset(buf,'\0',BUFMAXLEN);

	}
	close(c_s);
}