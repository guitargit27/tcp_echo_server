#include <ctype.h>
#include <errno.h>
#include <msgpack.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SA struct sockaddr
#define MAX_READ_SIZE 1024

typedef struct read_data
{
    int size;
    char* data;
} read_data;

void print_usage()
{
    printf ("\nTCP Echo Server\n\n");
    printf ("-p :   Mandatory: port number between 1-65535\n");
    printf ("-t :   Optional: timeout value, greater than 0.\n");
    printf ("         Defaults to 180s\n");
}

// Handle the message transactions between client and server
void echo_server(int connfd)
{
    //char* buffer = malloc(MAX_READ_SIZE);
    msgpack_sbuffer* mbuffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(mbuffer, msgpack_sbuffer_write);
    char* buffer = malloc(MAX_READ_SIZE);
    uint32_t buffer_size = 0;
    int bytes_read = 0;

    for (;;) {
        //memset (buffer, 0, MAX_READ_SIZE);
        msgpack_sbuffer_clear(mbuffer);
        msgpack_pack_array(pk, 2);

        // Read size
        //read (connfd, buffer, MAX_READ_SIZE);
        bytes_read = read (connfd, (char*)&buffer_size, sizeof(buffer_size));
        if (bytes_read > 0)
        {
            buffer_size = ntohl(buffer_size);

            if (buffer_size > 0)
            {
                printf ("size: %i\n", buffer_size);
            } else
            {
                write(connfd, "ERR", 4);
                fprintf (stderr, "Couldn't obtain size of data, exiting");
                break;
            }
        } else
        {
            write(connfd, "ERR", 4);
            fprintf (stderr, "Couldn't obtain size of data, exiting");
            break;
        }

        for (;;)
        {
            bytes_read = read (connfd, buffer, MAX_READ_SIZE);
            write(connfd, buffer, bytes_read);
            buffer_size -= bytes_read;
            // if (buffer_size <= 0)
            // {
            //     break;
            // }
            break;
        }

        /* deserializes it. */
        // msgpack_unpacked msg;
        // msgpack_unpacked_init(&msg);
        // msgpack_unpack_return ret = msgpack_unpack_next(&msg, mbuffer->data, mbuffer->size, NULL);

        // /* prints the deserialized object. */
        // msgpack_object obj = msg.data;
        // msgpack_object_print(stdout, obj);  /*=> ["Hello", "MessagePack"] */

        // Read data
        //write (connfd, mbuffer, MAX_READ_SIZE);
    }

    msgpack_sbuffer_free(mbuffer);
    msgpack_packer_free(pk);
    //free(buffer);
}

// Setup the echo server socket connection
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
    // TODO: change this to memset
    bzero(&servaddr, sizeof(servaddr));

    // Bind socket to IP and Port
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // setup socket
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

    len = sizeof(cli);
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0)
    {
        fprintf (stderr, "Couldn't accept on socket\n");
        exit (EXIT_FAILURE);
    }
    else
    {
        printf ("Accepted Client\n");
    }

    // Handle echo server
    echo_server(connfd);

    close (sockfd);
}

int main(int argc, char **argv)
{
    int port = 0;
    int timeout = 180;
    int options = 0;

    // Get Arguments and validate inputs
    while ((options = getopt (argc, argv, "p:t:?")) != -1)
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
            // timeout value
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
