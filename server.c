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

void print_usage() 
{
    printf ("\nTCP Echo Server\n\n");
    printf ("-p :   Mandatory: port number between 1-65535\n");
    printf ("-t :   Optional: timeout value, greater than 0.\n");
    printf ("         Defaults to 180s\n");
}

int main(int argc, char **argv) 
{
    int port = 0;
    int timeout = 180;
    int options = 0;

    // Get Arguments and validate inputs
    while ((options = getopt (argc, argv, "pt:?")) != -1)
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

    if (port <= 0) 
    {
        print_usage();
        exit (EXIT_FAILURE);          
    } else
    {
        printf ("Listening on port: %d\n", port);
    }

    return 0;
}
