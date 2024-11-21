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
	serv.sin_family = AF_INET;
	serv.sin_port = htons(8888);
	inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr.s_addr);
	
	char buf[1024];
	int n = 0;
	while(1){
		memset(buf, 0x00, sizeof(buf));
		n = read(STDIN_FILENO, buf, sizeof(buf));

		sendto(cfd, buf, n, 0, (struct sockaddr*)&serv, sizeof(serv));

		memset(buf, 0x00, sizeof(buf));
		recvfrom(cfd, buf, n, 0, NULL, NULL);

		printf("%s", buf);
	}
	close(cfd);
	return 0;
}
