#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <ctype.h>
#include <event2/event.h>
#include <netinet/in.h>

//struct event* connev = NULL;

struct event** myevent;

void readf(evutil_socket_t fd, short events, void *arg){
	int n = 0;
	char buf[1024];
	
	n = read(fd, buf, sizeof(buf));
	if(n <= 0){
		printf("read error or client closed fd : [%d]\n", fd);

	
		close(fd);
		event_del(myevent[fd]);	
		myevent[fd] = NULL;
		//event_free(events);
		
		//event_base_loopexit((struct event_base*)arg, NULL);
	}
	else{
		for(int i = 0; i < n; ++i) buf[i] = toupper(buf[i]);
		write(fd, buf, n);
	}
}
void connf(evutil_socket_t fd, short events, void *arg){
	int cfd = accept (fd, NULL, NULL);
	if(cfd <= 0){
		printf("accept error\n");
		//event_base_loopexit((struct event_base*)arg);
	}
	else{
		struct event* tmpevt = event_new((struct event_base*)arg, cfd, EV_READ | EV_PERSIST, readf, NULL);
		myevent[cfd]  = tmpevt;
		printf("new fd:[%d]\n", cfd);
		if(!myevent[cfd]){
			perror("event_new error");
		}
		else{
			event_add(myevent[cfd], NULL);
		}
	}
}

int main(){
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if(lfd < 0){
		perror("socket error\n");
		return -1;
	}
	
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	struct sockaddr_in serv;
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.sin_port = htons(8888);
	int ret = bind(lfd, (struct sockaddr*)&serv, sizeof(serv));
	if(ret < 0){
		perror("bind error");
		return -1;
	}
	
	listen(lfd, 128);

	struct event_base *base = event_base_new();
	if(!base){
		perror("event error");
		return -1;
	}
	
	struct event* ev = event_new(base, lfd, EV_READ | EV_PERSIST, connf, base);
	if(!ev){
		perror("event_new error");
		return -1;
	}

	event_add(ev, NULL);

	myevent = (struct event**)malloc(sizeof(struct event*) * 1024);

	event_base_dispatch(base);
	
	event_base_free(base);
	event_free(ev);

	close(lfd);
	
	return 0;
}
