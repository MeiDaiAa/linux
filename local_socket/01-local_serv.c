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

	unlink("./test.sock");
	
	struct sockaddr_un client;
	struct sockaddr_un serv;
	serv.sun_family = AF_UNIX;
	strcpy(serv.sun_path, "./test.sock");
	int ret = bind(lfd, (struct sockaddr*)&serv, sizeof(serv));
	if(ret < 0){
		perror("bind error");
		return -1;
	}
	
	listen(lfd, 128);
	
	socklen_t len = sizeof(client);
	int cfd = accept(lfd, (struct sockaddr*)&client, &len);
	if(cfd < 0){
		perror("accept error");
		return -1;
	}
	printf("link:[%s]\n", client.sun_path);
	
	int n = 0;
	char buf[1024];
	while(1){
		memset(buf, 0x00, sizeof(buf));

		n = read(cfd, buf, sizeof(buf));
		if(n <= 0){
			printf("read error or client closed\n");
			break;
		}
		printf("[%d][%s]\n", n, buf);

		for(int i = 0; i < n; ++i) buf[i] = toupper(buf[i]);
		
		write(cfd, buf, n);
	}
	close(lfd);
	close(cfd);
	
	return 0;
}
