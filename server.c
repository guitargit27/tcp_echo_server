#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SA struct sockaddr
#define read_buffer_size 1024

void print_usage() 
{
    printf ("\nTCP Echo Server\n\n");
    printf ("-p :   Mandatory: port number between 1-65535\n");
    printf ("-t :   Optional: timeout value, greater than 0.\n");
    printf ("         Defaults to 180s\n");
}

void echo_server(int sockfd)
{
    char* buffer = malloc(read_buffer_size);
    
    for (;;) {
        memset (buffer, 0, read_buffer_size);

        read (sockfd, buffer, sizeof(buffer));
        write (sockfd, buffer, sizeof(buffer));
    }
}


void setup_server(int port)
{
    int sockfd, connfd, len;
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
    bzero(&servaddr, sizeof(servaddr));

    // Bind socket to IP and Port
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0)
    {
        fprintf (stderr, "Couldn't bind socket\n");
        exit (EXIT_FAILURE);
    }
    else 
    {
        printf ("Socket bind successful\n");
    }

    if ((listen(sockfd, 5)) != 0)
    {
        fprintf (stderr, "Couldn't listen on socket\n");
        exit (EXIT_FAILURE);
    }
    else 
    {
        printf ("Socket listen successful\n");
    }        

    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd <0) 
    {
        fprintf (stderr, "Couldn't accept on socket\n");
        exit (EXIT_FAILURE);
    }
    else 
    {
        printf ("Accepted Client\n");
    }                
    
    echo_server(connfd);

    close (sockfd);
}

int main(int argc, char **argv) 
{
    int port = 0;
    int timeout = 180;
    int options = 0;
    extern char *optarg;

    // Get Arguments and validate inputs
    while ((options = getopt (argc, argv, "p:t:?")) != -1)
    {
        switch (options)
        {
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
            case 't':
                if (isdigit(*optarg))
                {
                    // still not quite right, will accept
                    // non-integers as long as they start with int
                    timeout = strtol(optarg, NULL, 10);
                } else
                {
                    fprintf (stderr, "Invalid timeout value\n");
                    print_usage();
                    exit (EXIT_FAILURE);                      

                }
                break;                
            case '?':
                print_usage();
                exit (EXIT_SUCCESS);                      

        }
    }

    if ((port <= 0) || (argc < 2))
    {
        print_usage();
        exit (EXIT_FAILURE);          
    } else
    {
        setup_server(port);
    }

    return 0;
}
