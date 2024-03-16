#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main()
{
    int server_fd, client1_socket, client2_socket, max_sd, activity, valread;
    int sd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    fd_set readfds;

    // 建立socket描述符
    //AF_INET是Address Family的縮寫，代表IPv4位址族
    //SOCK_STREAM表示使用TCP協定的
    //IPPROTO_TCP表示TCP協定，可填預設值0
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 設置socket選項（可選）
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 將socket綁定到指定的IP地址和端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 監聽連接
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 接受連接
    if ((client1_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Client 1 connected\n");
        
    // 接受連接
    if ((client2_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Client 2 connected\n");


    // 使用無窮迴圈持續監聽訊息
    while (1)
    {
        // 清空文件描述符集合
        FD_ZERO(&readfds);

        // 將兩個客戶端socket加入集合
        FD_SET(client1_socket, &readfds);
        FD_SET(client2_socket, &readfds);
        max_sd = (client1_socket > client2_socket) ? client1_socket : client2_socket;

        // 等待讀取事件發生
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
        {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        // 檢查client1是否有新訊息
        if (FD_ISSET(client1_socket, &readfds))
        {
            valread = read(client1_socket, buffer, 1024);
            if (valread == 0)
            {
                printf("Client 1 disconnected\n");
                break;
            }

            printf("Received from Client 1: %s\n", buffer);

	    // 將訊息從client1傳送到client2
	    send(client2_socket, buffer, strlen(buffer), 0);
	    printf("Sent to Client 2: %s\n", buffer);
	    memset(buffer, 0, sizeof(buffer));
	}
	
	// 檢查client2是否有新訊息
	if (FD_ISSET(client2_socket, &readfds))
	{
 	   valread = read(client2_socket, buffer, 1024);
 	   if (valread == 0)
  	  {
   	     printf("Client 2 disconnected\n");
   	     break;
  	  }

  	  printf("Received from Client 2: %s\n", buffer);

  	  // 將訊息傳送到client1
  	  send(client1_socket, buffer, strlen(buffer), 0);
  	  printf("Sent to Client 1: %s\n", buffer);
   	 memset(buffer, 0, sizeof(buffer));
   	}
   }
}
