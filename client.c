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

// pack string buffer using msgpack
//
// buffer  :    text input to pack
// full_size :  size of total buffer (msgpack serialized
//                data plus a int to store length)
// return   :   buf [uint32_t len, msgpack packed buffer]
static char *pack(char* buffer, uint32_t *full_size)
{
    char *buf = NULL;
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer pck;
    msgpack_packer_init(&pck, &sbuf, msgpack_sbuffer_write);

    // pack the user input length, followed by "Text"
    // followed by the user string input
    // format is arbitrary, just wanted to test some features
    msgpack_pack_array(&pck, 3);
    msgpack_pack_int32(&pck, strlen(buffer));
    msgpack_pack_bin(&pck, 4);
    msgpack_pack_bin_body(&pck, "Text", 4);
    msgpack_pack_bin(&pck, strlen(buffer));
    msgpack_pack_bin_body(&pck, buffer, strlen(buffer));

    // build a buffer that contains the full data size
    // to send over wire, basically all the msgpack stuff
    // and prepend the length of the buffer for the server
    *full_size = sbuf.size + sizeof(MAX_SIZE) + 1;
    buf = malloc(*full_size);
    uint32_t net_size = htonl(*full_size);
    memcpy(buf, (char *)&net_size, sizeof(MAX_SIZE));
    memcpy(buf + sizeof(MAX_SIZE), sbuf.data, sbuf.size);
    msgpack_sbuffer_destroy(&sbuf);
    printf ("Size: %u\n", *full_size);

    return buf;
}

// unpack buffer using msgpack
//
// buffer  :    msgpack data to unpack
// size    :    size of msgpack data to unpack
// return   :   data, original string entered from client
char* unpack(char* buffer, uint32_t size) {
    msgpack_unpacker* unp = msgpack_unpacker_new(size);
    memcpy(msgpack_unpacker_buffer(unp), buffer, size);
    msgpack_unpacker_buffer_consumed(unp, size);

    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    char *data;
    int echo_length = 0;

    // unpack the individual msgpack object elements
    // TODO: remove hardcoded element indices
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
void echo_client(int sockfd)
{
    int send_buffer_size = 1024;
    char* buffer = malloc(send_buffer_size);
    char* buffer_address = buffer;
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

        if (i != 0)
        {
            // Write user input out to server
            buffer[i] = '\0';

            // test pack
            char* send_buffer = malloc(send_buffer_size);
            memset (send_buffer, 0, send_buffer_size);
            send_buffer = pack(buffer, &size);
            printf ("pack %lu: %d bytes\n", strlen(send_buffer), size);

            write (sockfd, send_buffer, size);
            free(send_buffer);

            // read all bytes from the server
            bytes_read = 0;
            int c = 0;
            buffer_address = buffer;
            memset(buffer, 0, send_buffer_size);
            while (bytes_read < (size - sizeof(MAX_SIZE)))
            {
                c = read (sockfd, buffer_address, send_buffer_size - bytes_read);
                bytes_read += c;
                buffer_address += c;
                printf ("Bytes read: %i\n", bytes_read);
            }

            // unpack and echo output to the screen
            char* echo = malloc(send_buffer_size);
            memset (echo, 0, send_buffer_size);
            echo = unpack(buffer, size - sizeof(MAX_SIZE));
            printf ("Echoed text: %s\n", echo);
            free(echo);
        }
    }

    free(buffer);
}

// setup the client socket connection to the server
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

    // Echo Client
    echo_client(sockfd);

    close(sockfd);
}

// accept user inputs and launch the client
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

     if ((port <= 0) || (argc < 3))
    {
        print_usage();
        exit (EXIT_FAILURE);
    } else
    {
        setup_client(port, ip_addr);
    }

    return 0;
}
