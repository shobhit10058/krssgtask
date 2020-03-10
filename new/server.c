#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/in.h>
#include <unistd.h>
int *arr;
int n,sum=0,start=0,end,minion,i;
typedef struct
{
	int sock;
	struct sockaddr address;
	int addr_len;
} connection_t;
void * work(void * ptr)
{
	start = 0;
	end = (n-1);
       // printf("\n start = %d\n end = %d \n",start,end);
        int p_sum;
        connection_t * conn;
        long addr = 0;

        if (!ptr) pthread_exit(0); 
        conn = (connection_t *)ptr;
        /* read length of message */
        send(conn->sock, &n , sizeof(int),0);
        send(conn->sock, arr  , n*sizeof(int),0);
        send(conn->sock, &start, sizeof(int),0);
        send(conn->sock, &end, sizeof(int),0);
        addr = (long)((struct sockaddr_in *)&conn->address)->sin_addr.s_addr;
        recv(conn->sock, &p_sum , sizeof(int),0);
        //sum+=p_sum;
	for(i=0;i<n;i++)
	{
		sum+=arr[i];
	}
        printf("\n sum = %d \n",sum);
                /*printf("%d.%d.%d.%d: %s\n",
                (int)((addr      ) & 0xff),
                (int)((addr >>  8) & 0xff),
                (int)((addr >> 16) & 0xff),
                (int)((addr >> 24) & 0xff),
                buffer);
                free(buffer);*/
       // start=end+1;
        close(conn->sock);
        free(conn);
        pthread_exit(0);

}
void * process(void * ptr)
{
	if(i == minion-1)
	{
		end= (n-1);
	}
	else
	{
		end=start+ (n)/minion -1 ;
	}
	//printf("\n start = %d\n end = %d \n",start,end);
	int p_sum;
	connection_t * conn;
	long addr = 0;

	if (!ptr) pthread_exit(0); 
	conn = (connection_t *)ptr;
	/* read length of message */
	send(conn->sock, &n , sizeof(int),0);
	send(conn->sock, arr  , n*sizeof(int),0);
	send(conn->sock, &start, sizeof(int),0);
	send(conn->sock, &end, sizeof(int),0);
	addr = (long)((struct sockaddr_in *)&conn->address)->sin_addr.s_addr;
	recv(conn->sock, &p_sum , sizeof(int),0);
	sum+=p_sum;
	//printf("\n sum = %d \n",sum);
		/*printf("%d.%d.%d.%d: %s\n",
		(int)((addr      ) & 0xff),
		(int)((addr >>  8) & 0xff),
		(int)((addr >> 16) & 0xff),
		(int)((addr >> 24) & 0xff),
		buffer);
		free(buffer);*/
	start=end+1;
	close(conn->sock);
	free(conn);
	pthread_exit(0);
}
int main(int argc, char ** argv)
{
	int sock = -1;
	struct sockaddr_in address;
	int port;
	connection_t * connection;
	connection_t * connect;
	pthread_t thread,orgthread;
	/* check for command line arguments */
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s port\n", argv[0]);
		return -1;
	}
	/* obtain port number */
	if (sscanf(argv[1], "%d", &port) <= 0)
	{
		fprintf(stderr, "%s: error: wrong parameter: port\n", argv[0]);
		return -2;
	}
	/* create socket */
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock <= 0)
	{
		fprintf(stderr, "%s: error: cannot create socket\n", argv[0]);
		return -3;
	}
	/* bind socket to port */
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
	{
		fprintf(stderr, "%s: error: cannot bind socket to port %d\n", argv[0], port);
		return -4;
	}
	/* listen on port */
	if (listen(sock, 5) < 0)
	{
		fprintf(stderr, "%s: error: cannot listen on port\n", argv[0]);
		return -5;
	}
	printf("%s: ready and listening\n", argv[0]);
	connect = (connection_t *)malloc(sizeof(connection_t));
	//accepting client
	connect->sock = accept(sock, &connect->address, &connect->addr_len);	
	if(connect->sock < 0)
	{
		printf("accept failed by client\n");
		return -6;
	}
	if(recv(connect->sock,&n ,sizeof(int),0)<0)
	{
		printf("recv n failed");
		return -7;
	}
	int j;
	arr = (int *)malloc(n*sizeof(int));
	//printf("n = %d\n",n);
        if(recv(connect->sock, &minion ,sizeof(int),0)<0)
	{
		printf("recv minion failed");
		return -9;
	}
	printf("minion = %d",minion);
	if(recv(connect->sock, arr , n*sizeof(int),0)<0)
	{
		printf("recv arr failed");
		return -8;
	}
	if(minion == 1)
	{
		connection = (connection_t *)malloc(sizeof(connection_t));
                connection->sock = accept(sock, &connection->address, &connection->addr_len);
                if (connection->sock <= 0)
                {
                        free(connection);
                }
                else
                {
                        pthread_create(&thread, 0, work, (void *)connection);
                        pthread_detach(thread);
                }

	}
	else
	{
	for(i=0;i<minion;)
	{
		/* accept minions connections */
		connection = (connection_t *)malloc(sizeof(connection_t));
		connection->sock = accept(sock, &connection->address, &connection->addr_len);
		if (connection->sock <= 0)
		{
			free(connection);
		}
		else
		{
			pthread_create(&thread, 0, process, (void *)connection);
			pthread_detach(thread);
		}
		i++;
	}
	}
	//printf("\n final sum = %d",sum);
	if(send(connect->sock, &sum , sizeof(int),0)<0)
	{
		printf("send sum failed");
		return -9;
	}
	return 0;
}
