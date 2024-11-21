#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(){
	int cfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(cfd < 0){
		perror("socket error");
		return -1;
	}
	
	struct sockaddr_in serv;
	struct sockaddr_in client;
	serv.sin_family = AF_INET;
	serv.sin_port = htons(8888);
	//inet_ntop(AF_INET, "127.0.0.1", (struct sockaddr*)serv, sizeof(serv));
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(cfd, (struct sockaddr*)&serv, sizeof(serv));
	
	char buf[1024];
	int n = 0;
	socklen_t len = sizeof(client);
	while(1){
		memset(buf, 0x00, sizeof(buf));
		n = recvfrom(cfd, buf, sizeof(buf), 0, (struct sockaddr*)&client, &len);
		printf("[%d]:[%s]\n", ntohs(client.sin_port), buf);

		for(int i = 0; i < n; ++i) buf[i] = toupper(buf[i]);
		
		sendto(cfd, buf, n, 0, (struct sockaddr*)&client, len);
	}
	close(cfd);
	return 0;
}
