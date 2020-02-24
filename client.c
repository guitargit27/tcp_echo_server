#include <ctype.h>
#include <errno.h>
#include <msgpack.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SA struct sockaddr
#define MAX_SIZE 16384

static char *pack(char* buffer, uint32_t *size);

int isValidIPAddress(char *ip_addr)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip_addr, &(sa.sin_addr));

    return result;
}

static char *pack(char* buffer, uint32_t *size) {

    // Pack data into msgpack format
    char *buf = NULL;
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer pck;
    msgpack_packer_init(&pck, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pck, 3);
    msgpack_pack_int32(&pck, strlen(buffer));
    msgpack_pack_bin(&pck, 4);
    msgpack_pack_bin_body(&pck, "Text", 4);
    msgpack_pack_bin(&pck, strlen(buffer));
    msgpack_pack_bin_body(&pck, buffer, strlen(buffer));

    *size = sbuf.size + sizeof(MAX_SIZE) + 1;
    buf = malloc(*size);
    uint32_t net_size = htonl(*size);
    memcpy(buf, (char *)&net_size, sizeof(MAX_SIZE));
    memcpy(buf + sizeof(MAX_SIZE), sbuf.data, sbuf.size);
    msgpack_sbuffer_destroy(&sbuf);
    printf ("Size: %u\n", *size);

    return buf;
}

char* unpack(char* buffer, uint32_t size) {
    msgpack_unpacker* unp = msgpack_unpacker_new(size);
    memcpy(msgpack_unpacker_buffer(unp), buffer, size);
    msgpack_unpacker_buffer_consumed(unp, size);

    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    char *data;
    int echo_length = 0;

    if (msgpack_unpacker_next(unp, &msg))
    {
        msgpack_object root = msg.data;
        if (root.type == MSGPACK_OBJECT_ARRAY) {
            echo_length = root.via.array.ptr[0].via.i64;
            if (echo_length > 0)
            {
                data = malloc(echo_length);
                strncpy(data, root.via.array.ptr[2].via.str.ptr, echo_length);
            }
        }
    }

    msgpack_unpacked_destroy(&msg);
    return data;
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
    char* echo;
    int bytes_read = 0;

    uint32_t size = 0;

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
                send_buffer_size = MAX_SIZE;
                buffer = realloc(buffer, send_buffer_size);
            }
            if (i > MAX_SIZE)
            {
                fprintf (stderr, "Buffer too large to send, exiting\n");

                free(buffer);
                return;
            }
        }

        // Write user input out to server
        buffer[i] = '\0';

        // test pack
        char* send_buffer = malloc(send_buffer_size);
        memset (send_buffer, 0, send_buffer_size);
        send_buffer = pack(buffer, &size);
        printf ("pack %lu: %d bytes\n", strlen(send_buffer), size);

        write (sockfd, send_buffer, size);
        free(send_buffer);

        bytes_read = read (sockfd, buffer, send_buffer_size);
        printf ("Bytes read: %i\n", bytes_read);

        char* echo = malloc(send_buffer_size);
        memset (echo, 0, send_buffer_size);
        echo = unpack(buffer, size - sizeof(MAX_SIZE));
        printf ("Echoed text: %s\n", echo);
        free(echo);
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
