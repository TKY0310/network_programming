#include <sys/socket.h>
#include<fcntl.h>
#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<netdb.h>
#include <sys/types.h>

#define BUFMAXLEN 1024
#define PORT_NO 10034
#define MAX_SPLIT_LEN 5
#define MAX_STR_LEN 69


int subst(char *str, char c1, char c2){
	int n = 0;
	while(*str){
		if(*str == c1){
			*str = c2;
			n++;
		}
		str++;
	}
	return n;
}

int split(char *str, char *ret[], char sep, int max){
	int n = 0;
	ret[n++] = str;
	while(*str && n < max){
		if(*str == sep){
			*str = '\0';
			ret[n++] = str + 1;
		}
		str++;
	}
	return n;
}

int get_line(FILE *fp, char *line){
	if(fgets(line, BUFMAXLEN + 1, fp) == NULL){
	return 0;
	}
	subst(line, '\n', '\0');

	return 1;
}

void parse_line(int new_s,char *line,char*message,char*buf);

struct date{
	int y;
	int m;
	int d;
};

struct date *new_date(struct date *d,char *str){
	char *day[3];
	if(split(str,day,'-',3) != 3) return NULL;
	d->y = atoi(day[0]);
	d->m = atoi(day[1]);
	d->d = atoi(day[2]);
	return d;
}

struct profile{
	int Id;
	char Name[MAX_STR_LEN + 1];
	struct date Birth;
	char Addr[MAX_STR_LEN + 1];
	char *Cmd;
};

struct profile profile_data_store[10000];

int profile_data_nitems = 0;

int count = 0;

struct profile *new_profile(int new_s,struct profile *P ,char *line, char *message){
	char *ret[5];
		
	if(split(line,ret,',',MAX_SPLIT_LEN) != MAX_SPLIT_LEN) {
		fprintf(stderr, "error: not create new profile\n");
		sprintf(message, "not create new profile\n");
		send(new_s,message,strlen(message),0);
		return;
	}
	
	P->Id = atoi(ret[0]);
	strncpy (P->Name,ret[1],MAX_STR_LEN);
	P->Name[MAX_STR_LEN] = '\0';
	if(new_date(&P->Birth,ret[2]) == NULL) return NULL;
	strncpy (P->Addr,ret[3],MAX_STR_LEN);
	P->Addr[MAX_STR_LEN] = '\0';

	P->Cmd = (char *)malloc(sizeof(char) * (strlen(ret[4])+1));
	strcpy (P->Cmd,ret[4]);
	
	sprintf(message, "Create new profile\n");
	fprintf(stderr, "Create new profile\n");
	profile_data_nitems++;
	
	send(new_s,message,strlen(message),0);

	return P;
}

/*%P*/
void print_profile(int new_s,struct profile *P,char*message,char*buf){
	memset(message,'\0',BUFMAXLEN);
	sprintf(message, "Id   : %d\nName : %s\nBirth: %d-%02d-%02d\nAddr : %s\nCom. : %s\n\n",P->Id, P->Name, P->Birth.y ,P->Birth.m ,P->Birth.d, P->Addr, P->Cmd);
	send(new_s,message,strlen(message),0);
	recv(new_s,buf,sizeof(buf),0);
	if(strcmp(buf,"ACK") != 0){
		fprintf(stderr,"recv() Error on print profile\n");
		return;
	}
}

/*%W*/
void fprint_profile_csv(FILE *fp,struct profile *P){
	fprintf(fp,"%d,%s,%d-%02d-%02d,%s,%s\n",P->Id, P->Name, P->Birth.y ,P->Birth.m ,P->Birth.d, P->Addr, P->Cmd);
}

/*command*/
void cmd_quit(int new_s,char *message){
	fprintf(stderr, "end\n");
	sprintf(message,"end\n");
	send(new_s,message,strlen(message),0);
	close(new_s);
}

void cmd_check(int new_s,char *message){
	fprintf(stderr, "Show profiles number\n");
	sprintf(message,"%d profile(s)\n",profile_data_nitems);
	send(new_s,message,strlen(message),0);
}

void cmd_print(int new_s,int nitems,char *message,char *buf){
	int i = 0;
	
	if(profile_data_nitems < abs(nitems)){
	fprintf(stderr, "error: over the number\n");
	sprintf(message, "error: over the number\n");
	send(new_s,message,strlen(message),0);
	return;
	}
	
	memset(message,'\0',BUFMAXLEN);
	memset(buf,'\0',BUFMAXLEN);
	
	fprintf(stderr, "show profiles\n");
	sprintf(message,"show profiles\n");
	send(new_s,message,strlen(message),0);
	recv(new_s,buf,sizeof(buf),0);
	if(strcmp(buf,"ACK") != 0){
		fprintf(stderr,"recv() Error on cmd_print\n");
		return;
	}
	
	if(nitems > 0){
		while(i < nitems){
			print_profile(new_s,&profile_data_store[i++],message,buf);
			memset(buf,'\0',BUFMAXLEN);
		}
		memset(message,'\0',BUFMAXLEN);
		sprintf(message,"END\n");
		send(new_s,message,strlen(message),0);
	}

	else if(nitems == '\0' || profile_data_nitems == abs(nitems)){
		while(i < profile_data_nitems){
			print_profile(new_s,&profile_data_store[i++],message,buf);
			memset(buf,'\0',BUFMAXLEN);
		}
		memset(message,'\0',BUFMAXLEN);
		sprintf(message,"END\n");
		send(new_s,message,strlen(message),0);
	}

	else if(nitems < 0){
		while(i < abs(nitems)){
			print_profile(new_s,&profile_data_store[profile_data_nitems + nitems + i++],message,buf);
			memset(buf,'\0',BUFMAXLEN);
		}
		memset(message,'\0',BUFMAXLEN);
		sprintf(message,"END\n");
		send(new_s,message,strlen(message),0);
	}
}

void cmd_read(int new_s,char *filename,char *message,char*buf){

	FILE* fp;
	char line[BUFMAXLEN + 1];
	
	if (strstr(filename,"\n")){
	subst(filename, '\n', '\0');
	}
	
	fprintf(stderr, "read to %s\n",filename);
	
	fp = fopen(filename,"r");
	
	if(fp == NULL){
		fprintf(stderr,"Error: open() failed\n");
		sprintf(message, "error: cannot open the file\n");
		send(new_s,message,strlen(message),0);
		return;
	}
	
	memset(message,'\0',BUFMAXLEN);
	memset(buf,'\0',BUFMAXLEN);
	
	sprintf(message,"read file\n");
	send(new_s,message,strlen(message),0);
	recv(new_s,buf,sizeof(buf),0);
	if(strcmp(buf,"OK") != 0){
		fprintf(stderr,"recv() Error on cmd_read\n");
		return;
	}
	
	while(get_line(fp, line)){
		memset(buf,'\0',BUFMAXLEN);
		memset(message,'\0',BUFMAXLEN);
		fprintf(stderr, "line %d:\n", ++count);
		parse_line(new_s,line,message,buf);
		if(strcmp(message,"end\n") == 0) break;
		recv(new_s,buf,sizeof(buf),0);
		if(strcmp(buf,"OK") != 0){
		fprintf(stderr,"recv() Error on cmd_read_while\n");
		return;
	}
	}
	if(strcmp(message,"end\n") == 0){
		
	}
	else{
	fprintf(stderr, "read all text\n");
	sprintf(message, "read all text\n");
	send(new_s,message,strlen(message),0);
	}
	fclose(fp);
}

void cmd_write(int new_s,char *message,char *filename){
	if (filename == NULL){
		filename = "test.txt";
	}
	FILE *fp;
	int i;
	subst(filename, '\n', '\0');
	fp = fopen(filename,"w");
	
	if (fp == NULL) {
		fprintf(stderr, "Could not open file: %s\n", filename);
		sprintf(message,"Could not open file: %s\n", filename);
		send(new_s,message,strlen(message),0);
		return;
	}

	for(i=0; i < profile_data_nitems; i++){
		fprint_profile_csv(fp, &profile_data_store[i]);
	}
	fclose(fp);
	fprintf(stderr,"write %s\n",filename);
	sprintf(message,"write to %s\n",filename);
	send(new_s,message,strlen(message),0);
}

void exec_command(int new_s,char *cmd, char *param, char*message,char*buf){
	if(*cmd >= 97 && *cmd <= 122){
	*cmd = *cmd -32;
	}

	switch (*cmd) {
	case 'Q': cmd_quit(new_s,message); break;
	case 'C': cmd_check(new_s,message); break;
	case 'P': cmd_print(new_s,atoi(param),message,buf); break;
	case 'R': cmd_read(new_s,param,message,buf); break;
	case 'W': cmd_write(new_s,message,param); break;
	default:
	fprintf(stderr, "Invalid command %s\n", cmd);
	sprintf(message,"Invalid command %s\n",cmd);
	send(new_s,message,strlen(message),0);
	break;
	}
}

void parse_line(int new_s,char *line,char*message,char*buf)
{
	if (line[0] == '%') {
	exec_command(new_s,&line[1], &line[3],message,buf);
	}
	else {
	new_profile(new_s,&profile_data_store[profile_data_nitems], line, message);
	}
}

int main(int argc,char*argv[]){	

	int s_s;
	int new_s;
	int size = BUFMAXLEN;
	struct sockaddr_in sa;
	char buf[BUFMAXLEN],message[BUFMAXLEN];
	memset(buf,'\0',BUFMAXLEN);
	memset(message,'\0',BUFMAXLEN);
	
	if((s_s = socket(AF_INET,SOCK_STREAM,0)) < 0){
		perror("Error: socket()");
		exit(1);
	}
	
	memset((char *) &sa, 0, sizeof(sa));
	sa.sin_family = AF_INET; 
	sa.sin_addr.s_addr = htonl(INADDR_ANY); 
	sa.sin_port = htons(PORT_NO); 
	
	if(bind(s_s, (struct sockaddr*)&sa, sizeof(sa))== -1){
		perror("Error; bind()");
		exit(1);
	}
	
	if(listen(s_s, 5) == -1){
		perror("Error; listen()");
		return -1;
	}
	
	while(1){
	new_s = accept(s_s, (struct sockaddr*)&sa, &size);
	if(new_s == -1){
			perror("Error; accept()");
			return -1;
		}

		while(1){
	int rlen = recv(new_s,buf,sizeof(buf),0);
	if(rlen < 0){
		perror("Error:recv()");
		exit(1);
	}
	else if(rlen == 0) break;
	
	parse_line(new_s,buf,message,buf);

	if(strcmp(message,"end\n")== 0)break;
		
	memset(buf,'\0',BUFMAXLEN);
	memset(message,'\0',BUFMAXLEN);
			
		}
		close(new_s);
	}
	close(s_s);
}

