#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "wrap.h"
#include "pub.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

int http_request(int sockfd, int epfd);
int send_header(int cfd, char *code, char *msg, char *fileType, int len);
int send_file(int cfd, char *fileName);
int send_list(int fd, char *pFile);
int send_dir(int fd, char *pFile);


int main(){
	// 改变当前的工作路径
	char path[255] = {0}; 
	sprintf(path, "%s/%s", getenv("HOME"), "webpath");
	chdir(path);

	// 创建监听文件描述符
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0) {
		perror("socket error");
		return -1;
	}
	// 设置端口复用
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	// 绑定
	struct sockaddr_in serv;
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = INADDR_ANY;
	serv.sin_port = htons(8888);
	if (bind(lfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
		perror("bind error");
		return -1;
	}

	// 监听
	if (listen(lfd, 128) < 0) {
		perror("listen error");
		return -1;
	}

	// 创建 epoll 树根节点
	int epfd = epoll_create(1);

	// 创建监听事件并将监听事件上树
	struct epoll_event ev, evs[1024];
	ev.data.fd = lfd;
	ev.events = EPOLLIN;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) < 0) {
		perror("epoll_ctl error");
		return -1;
	}

	struct sockaddr_in client;
	socklen_t len = sizeof(client);
	char sIP[16];
	memset(sIP, 0x00, sizeof(sIP));

	// 进入循环监听
	while (1) {
		int nready = epoll_wait(epfd, evs, 1024, -1); 
		if (nready <= 0) {
			if (errno == EINTR) continue;
			printf("epoll_wait error\n");
			break;
		}
		for (int i = 0; i < nready; ++i) {
			int sockfd = evs[i].data.fd;
			if (sockfd == lfd) {
				// 有新的连接到来
				int cfd = accept(lfd, (struct sockaddr*)&client, &len);
				if (cfd < 0) {
					printf("accept error\n");
					break;
				}
				// 打印出客户端信息
				printf("link:[%s]:[%d]\n", inet_ntop(AF_INET, &client.sin_addr.s_addr, sIP, sizeof(sIP)), ntohs(client.sin_port));

				// 设置 cfd 为非阻塞
				int flag = fcntl(cfd, F_GETFD);
				flag |= O_NONBLOCK;
				fcntl(cfd, F_SETFD);

				// 将新创建的通信文件描述符上树
				ev.data.fd = cfd;
				ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
			} else {
				// 处理通信文件描述符
				http_request(sockfd, epfd);
			}
		}
	}
	close(lfd);
	return 0;
}

int http_request(int sockfd, int epfd) {
	int n = 0;
	char buf[1024];
	memset(buf, 0x00, sizeof(buf));

	// 读取请求的第一行
	n = Readline(sockfd, buf, sizeof(buf));
	if (n <= 0) {
		printf("client closed or read error\n");
		close(sockfd);    
		epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
		return -1;
	}
	printf("[%s]\n", buf);

	// 解析请求的第一行
	char reqType[16] = {0};
	char fileName[255] = {0};
	char protocal[16] = {0};
	sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", reqType, fileName, protocal);
	printf("reqType:[%s]\n", reqType);
	printf("fileName:[%s]\n", fileName);
	printf("protocal:[%s]\n", protocal);


	char *pFile = fileName;
	//没有输入请求文件名的情况
	if(strlen(fileName) <= 1){
		strcpy(pFile, "./");
	}
	// 获取正常文件名，跳过 '/'
	else pFile = fileName + 1;

	printf("pFile:[%s]\n", pFile);

	// 丢弃剩余的请求头，直到遇到空行
	while ((n = Readline(sockfd, buf, sizeof(buf))) > 0) {
		if (strcmp(buf, "\r\n") == 0) {
			break;  // 遇到空行，说明请求头结束
		}
	}

	// 文件处理
	struct stat st;
	if (stat(pFile, &st) < 0) {
		// 文件不存在
		printf("file not exist\n");
		send_header(sockfd, "404", "NOT FOUND", get_mime_type("error.html"), 0);
		send_file(sockfd, "error.html");
	} else {
		// 文件存在
		if (S_ISREG(st.st_mode)) {
			//为普通文件
			printf("file exist\n");
			send_header(sockfd, "200", "OK", get_mime_type(pFile), st.st_size);
			send_file(sockfd, pFile);
		} else if (S_ISDIR(st.st_mode)) {
			// 为目录文件
			printf("目录文件\n");
			// 发送返回头
			send_header(sockfd, "200", "OK", get_mime_type(".html"), 0);

			//发送显示文件列表的网页 
			send_dir(sockfd, pFile);
		}
	}

	return 0;
}


int send_header(int cfd, char *code, char *msg, char *fileType, int len) {
	char buf[1024] = {0};
	sprintf(buf, "HTTP/1.1 %s %s\r\n", code, msg);
	sprintf(buf + strlen(buf), "Content-Type:%s\r\n", fileType);

	if (len > 0) {
		sprintf(buf + strlen(buf), "Content-Length:%d\r\n", len);
	}

	strcat(buf, "\r\n");
	Write(cfd, buf, strlen(buf));

	return 0;
}

int send_file(int cfd, char *fileName) {
	int fd = open(fileName, O_RDONLY);
	if (fd < 0) {
		printf("open error\n");
		return -1;
	}

	int n;
	char buf[1024];

	while (1) {
		memset(buf, 0x00, sizeof(buf));
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) {
			break;
		} else {
			Write(cfd, buf, n);  // 正确的字节数是 n 
		}
	}
	close(fd);
	return 0;
}


int send_dir(int fd, char *pFile){
	//先发送网页列表前半段
	send_file(fd, "html_list_head.txt");

	//发送真正的列表内容
	send_list(fd, pFile);


	//发送网页列表的后半段
	send_file(fd, "html_list_back.txt");
}

int send_list(int fd, char *pFile){
	struct dirent **namelist;
	int n;
	char buf[1024];

	n = scandir(pFile, &namelist, NULL, alphasort);
	if (n == -1) {
		perror("scandir");
		exit(EXIT_FAILURE);
	}

	memset(buf,0x00, sizeof(buf));
	while (n--) {
		printf("%s\n", namelist[n]->d_name);
		if(namelist[n]->d_type == DT_DIR)
			sprintf(buf + strlen(buf), "<li><a href = %s/>%s</a></li>", namelist[n]->d_name, namelist[n]->d_name);
		else
			sprintf(buf + strlen(buf), "<li><a href = %s>%s</a></li>", namelist[n]->d_name, namelist[n]->d_name);
			
		if(strlen(buf) > 1000) {
			Write(fd, buf, sizeof(buf));
			memset(buf, 0x00, sizeof(buf));
		}
		free(namelist[n]);
	}
	Write(fd, buf, sizeof(buf));
	
	free(namelist);
}

