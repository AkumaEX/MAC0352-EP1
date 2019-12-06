#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "functions.h"

#define LISTENQ 1
#define ERROR -1
#define CLOSED -1
#define TRUE 1
#define FALSE 0
#define BSIZE 150

int main(int argc, char *argv[])
{
    struct sockaddr_in server_cont, server_data, client, client_data;
    int listen_contfd, listen_datafd, contfd, datafd, userauth, connected;
    char buffer[BSIZE], command[BSIZE], username[BSIZE], message[BSIZE], mode[BSIZE];
    socklen_t bytes;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <server ip address> <listening port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    while (chdir("remote_data") == ERROR)
    {
        if (mkdir("remote_data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == ERROR)
        {
            print_error("chdir :(\n");
        }
    }

    /* SOCKET */
    if ((listen_contfd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
    {
        print_error("socket :(\n");
    }
    if ((listen_datafd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
    {
        print_error("data socket :(\n:");
    }

    /* STARTUP */
    bzero(&server_cont, sizeof(server_cont));
    server_cont.sin_family = AF_INET;
    server_cont.sin_addr.s_addr = htonl(INADDR_ANY);
    server_cont.sin_port = htons(atoi(argv[2]));
    bzero(&server_data, sizeof(server_data));
    server_data.sin_family = AF_INET;
    server_data.sin_addr.s_addr = htonl(INADDR_ANY);
    server_data.sin_port = 0;

    /* BIND */
    if (bind(listen_contfd, (struct sockaddr *)&server_cont, sizeof(server_cont)) == ERROR)
    {
        print_error("server bind :(\n");
    }
    if (bind(listen_datafd, (struct sockaddr *)&server_data, sizeof(server_data)) == ERROR)
    {
        print_error("server data bind :(\n");
    }

    /* LISTEN */
    if (listen(listen_contfd, LISTENQ) == ERROR)
    {
        print_error("server listen :(\n");
    }
    if (listen(listen_datafd, LISTENQ) == ERROR)
    {
        print_error("server data listen :(\n");
    }

    printf("Server Running\n");

    while (1)
    {
        /* NEW SOCKET */
        bytes = sizeof(client);
        if ((contfd = accept(listen_contfd, (struct sockaddr *)&client, &bytes)) == ERROR)
        {
            print_error("accept :(\n");
        }

        send_message(contfd, "200 MAC0352 EP1 Server\n");
        memset(username, 0, BSIZE);
        userauth = FALSE;
        connected = TRUE;
        datafd = CLOSED;

        while (connected)
        {
            memset(buffer, 0, BSIZE);
            memset(command, 0, BSIZE);
            bytes = read(contfd, buffer, BSIZE);
            sscanf(buffer, "%s", command);

            if (!strcasecmp(command, "QUIT"))
            {
                quit(contfd, &connected, username);
            }
            else if (!strcasecmp(command, "SYST"))
            {
                syst(contfd);
            }
            else if (!strcasecmp(command, "TYPE"))
            {
                type(contfd, buffer, mode);
            }
            else if (!strcasecmp(command, "USER"))
            {
                user(contfd, buffer, username);
            }
            else if (!strcasecmp(command, "PASS"))
            {
                pass(contfd, username, &userauth);
            }
            else if (!strcasecmp(command, "PASV"))
            {
                pasv(contfd, listen_datafd, argv[1], &userauth);
                bytes = sizeof(client_data);
                if ((datafd = accept(listen_datafd, (struct sockaddr *)&client_data, &bytes)) == ERROR)
                {
                    print_error("pasv :(\n");
                }
            }
            else if (!strcasecmp(command, "LIST"))
            {
                list(contfd, &datafd, &userauth);
            }
            else if (!strcasecmp(command, "RETR"))
            {
                retr(contfd, &datafd, buffer, mode, &userauth);
            }
            else if (!strcasecmp(command, "STOR"))
            {
                stor(contfd, &datafd, buffer, mode, &userauth);
            }
            else if (!strcasecmp(command, "DELE"))
            {
                dele(contfd, buffer, &userauth);
            }
            else
            {
                sprintf(message, "500 %s not understood\n", command);
                send_message(contfd, message);
            }
        }
    }
    close(listen_contfd);
    close(listen_datafd);
    return EXIT_SUCCESS;
}