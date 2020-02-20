#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SA struct sockaddr
#define max_size 16384

int isValidIPAddress(char *ip_addr)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip_addr, &(sa.sin_addr));

    return result;
}

void print_usage()
{
    printf ("\nTCP Echo Server Client\n\n");
    printf ("-p :   Mandatory: port number between 1-65535\n");
    printf ("-i :   Mandatory: IP address\n");
}

// Accept user input, send to Echo Server, print return message
// TODO: need to use something like msgpack to determine the data length
// Right now, would not read full message if data was larger than server buffer size
void chat_server(int sockfd)
{
    int send_buffer_size = 1024;
    char* buffer = malloc(send_buffer_size);
    int c = EOF;
    int i = 0;

    // Handle user input from the command prompt
    for (;;)
    {
        memset (buffer, 0, send_buffer_size);
        i = 0;
        send_buffer_size = 1024;

        printf ("> ");
        while ((c = getchar()) != '\n')
        {
            buffer[i] = c;
            i = i + 1;

            // realloc if max sized reached
            if (i >= send_buffer_size)
            {
                send_buffer_size = max_size;
                buffer = realloc(buffer, send_buffer_size);
            }
            if (i > max_size)
            {
                fprintf (stderr, "Buffer too large to send, exiting\n");

                free(buffer);
                return;
            }
        }

        // Write user input out to server
        buffer[i] = '\0';
        write (sockfd, buffer, strlen(buffer));

        // Read return from server, display to screen
        memset (buffer, 0, send_buffer_size);
        read (sockfd, buffer, send_buffer_size);
        printf ("Echoed message: %s\n", buffer);
    }

    free(buffer);
}

void setup_client(int port, char* ip_addr)
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        fprintf (stderr, "Failed to create socket, exiting\n");
        exit (EXIT_FAILURE);
    }
    else
    {
        printf ("Socket created successfully\n");
    }
    // TODO: change this to memset
    bzero(&servaddr, sizeof(servaddr));

    // Assign IP and Port to socket
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_addr);
    servaddr.sin_port = htons(port);

    // Connect to the server
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0)
    {
        fprintf (stderr, "Could not connect to server at %s:%d\n", ip_addr, port);
        exit (EXIT_FAILURE);
    } else
    {
        printf ("Connected to server at %s:%d\n", ip_addr, port);
    }

    // Chat Server
    chat_server(sockfd);

    close(sockfd);

}

int main(int argc, char **argv)
{
    int port = 0;

    int timeout = 180;
    int options = 0;
    char* ip_addr;

    // Get Arguments and validate inputs
    while ((options = getopt (argc, argv, "p:i:?")) != -1)
    {
        switch (options)
        {
            // port number
            case 'p':
                if (isdigit(*optarg))
                {
                    // still not quite right, will accept
                    // non-integers as long as they start with int
                    port = strtol(optarg, NULL, 10);
                    if ((port < 1) || (port > 65535))
                    {
                        fprintf (stderr, "Invalid port number\n");
                        print_usage();
                        exit (EXIT_FAILURE);
                    }
                } else
                {
                    fprintf (stderr, "Invalid port number\n");
                    print_usage();
                    exit (EXIT_FAILURE);

                }
                break;
            // ip address
            case 'i':
                if (isValidIPAddress(optarg))
                {
                    // still not quite right, will accept
                    // non-integers as long as they start with int
                    ip_addr = optarg;
                    printf ("%s", ip_addr);
                } else
                {
                    fprintf (stderr, "Invalid IP address\n");
                    print_usage();
                    exit (EXIT_FAILURE);
                }
                break;
            case '?':
                print_usage();
                exit (EXIT_SUCCESS);
        }
    }

    setup_client(port, ip_addr);

    return 0;
}
