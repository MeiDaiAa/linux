#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/un.h>

int main(){
	int lfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(lfd < 0){
		perror("socket error");
		return -1;
	}
	
	unlink("./client.sock");
	
	struct sockaddr_un client;
	client.sun_family = AF_UNIX;
	strcpy(client.sun_path, "./client.sock");
	int ret = bind(lfd, (struct sockaddr*)&client, sizeof(client));
	if(ret < 0){
		perror("bind error");
		return -1;
	}
	
	struct sockaddr_un serv;
	serv.sun_family = AF_UNIX;
	strcpy(serv.sun_path, "./test.sock");
	connect(lfd, (struct sockaddr*)&serv, sizeof(serv));
	if(lfd < 0){
		perror("connect error");	
		return -1;
	}
	
	int n = 0;
	char buf[1024];
	while(1){
		memset(buf, 0x00, sizeof(buf));

		n = read(STDIN_FILENO, buf, sizeof(buf));
		write(lfd, buf, n);
		
		memset(buf, 0x00, sizeof(buf));
		n = read(lfd, buf, sizeof(buf));
		
		printf("%s", buf);
	}

	close(lfd);
	return 0;
}
