#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> /*O_RDONLY*/
#include <sys/stat.h>
#include <sys/sendfile.h>
#include "functions.h"

#define ERROR -1
#define TRUE 1
#define CLOSED -1
#define FALSE 0
#define BSIZE 150

int send_message(int contfd, char *message)
{
    return write(contfd, message, strlen(message));
}

int print_error(char *message)
{
    perror(message);
    exit(1);
}

void user(int contfd, char *buffer, char *username)
{
    char command[BSIZE], message[BSIZE];
    sscanf(buffer, "%s%s", command, username);
    sprintf(message, "331 Password required for %s\n", username);
    send_message(contfd, message);
}

void pass(int contfd, char *username, int *userauth)
{
    char message[BSIZE];
    if (*username)
    {
        printf("230 %s Connected\n", username);
        sprintf(message, "230 User %s logged in\n", username);
        send_message(contfd, message);
        *userauth = TRUE;
    }
    else
    {
        send_message(contfd, "503 Login with USER first\n");
        *userauth = FALSE;
    }
}

void type(int contfd, char *buffer, char *mode)
{
    char command[BSIZE], parameter[BSIZE], message[BSIZE];
    memset(parameter, 0, BSIZE);
    sscanf(buffer, "%s%s", command, parameter);
    if (*parameter)
    {
        if (!strcmp(parameter, "I"))
        {
            send_message(contfd, "200 Type set to I\n");
            sprintf(mode, "BINARY");
        }
        else if (!strcmp(parameter, "A"))
        {
            send_message(contfd, "200 Type set to A\n");
            sprintf(mode, "ASCII");
        }
        else
        {
            sprintf(message, "504 TYPE not implemented for '%s' parameter\n", parameter);
            send_message(contfd, message);
        }
    }
    else
    {
        send_message(contfd, "500 'TYPE' not understood\n");
    }
}

void syst(int contfd)
{
    send_message(contfd, "215 UNIX Type: L8\n");
}

void pasv(int contfd, int listenfd_data, char *ip, int *userauth)
{
    if (userauth)
    {

        char message[BSIZE];
        int ip1, ip2, ip3, ip4, port;
        struct sockaddr_in server_info;
        socklen_t bytes = sizeof(server_info);

        if ((getsockname(listenfd_data, (struct sockaddr *)&server_info, &bytes)) == ERROR)
        {
            print_error("getsockname :(\n");
        }

        port = (int)ntohs(server_info.sin_port);

        sscanf(ip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

        sprintf(message, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", ip1, ip2, ip3, ip4, port / 256, port % 256);
        send_message(contfd, message);
    }
    else
    {
        send_message(contfd, "530 Please login with USER and PASS\n");
    }
}

void list(int contfd, int *datafd, int *userauth)
{
    if (userauth)
    {
        if (*datafd != CLOSED)
        {
            FILE *file;
            char line[BSIZE];
            int size;
            send_message(contfd, "150 Opening ASCII mode data connection for file list\n");

            if ((system("ls -la > ../list.txt")) == ERROR)
            {
                print_error("system :(\n");
            }

            file = fopen("../list.txt", "rb");
            while (fgets(line, BSIZE, file) != NULL)
            {
                size = strlen(line);
                line[size - 1] = '\r';
                line[size] = '\n';
                line[size + 1] = '\0';
                send_message(*datafd, line);
            }
            send_message(contfd, "226 Transfer complete\n");
            remove("../list.txt");
            fclose(file);
            close(*datafd);
            *datafd = CLOSED;
        }
        else
        {
            send_message(contfd, "425 Unable to build data connection: Connection refused\n");
            send_message(contfd, "450 LIST: Connection refused\n");
        }
    }
    else
    {
        send_message(contfd, "530 Please login with USER and PASS\n");
    }
}

void retr(int contfd, int *datafd, char *buffer, char *mode, int *userauth)
{
    if (userauth)
    {
        if (*datafd != CLOSED)
        {
            int filehandle;
            struct stat obj;
            char command[BSIZE], filename[BSIZE], message[BSIZE];

            sscanf(buffer, "%s%s", command, filename);
            stat(filename, &obj);
            if ((filehandle = open(filename, O_RDONLY)) == ERROR)
            {
                sprintf(message, "550 %s: No such file or directory\n", filename);
                send_message(contfd, message);
            }
            else
            {
                sprintf(message, "150 Opening %s mode data connection for %s\n", mode, filename);
                send_message(contfd, message);
                sendfile(*datafd, filehandle, NULL, obj.st_size);
                send_message(contfd, "226 Transfer complete\n");
                close(filehandle);
                close(*datafd);
                *datafd = CLOSED;
            }
        }
        else
        {
            send_message(contfd, "425 Unable to build data connection: Connection refused\n");
        }
    }
    else
    {
        send_message(contfd, "530 Please login with USER and PASS\n");
    }
}

void stor(int contfd, int *datafd, char *buffer, char *mode, int *userauth)
{
    if (userauth)
    {
        if (*datafd != CLOSED)
        {
            FILE *file;
            ssize_t bytes;
            char command[BSIZE], filename[BSIZE], message[BSIZE];
            sscanf(buffer, "%s%s", command, filename);

            if ((file = fopen(filename, "w")) == NULL)
            {
                send_message(contfd, "451 Requested action aborted. Local error in processing.\n");
            }
            else
            {
                sprintf(message, "150 Opening %s mode data connection for %s\n", mode, filename);
                send_message(contfd, message);
                do
                {
                    bytes = read(*datafd, buffer, BSIZE);
                    fwrite(buffer, 1, bytes, file);
                } while (bytes > 0);
                send_message(contfd, "226 Transfer complete\n");
                fclose(file);
                close(*datafd);
                *datafd = CLOSED;
            }
        }
        else
        {
            send_message(contfd, "425 Unable to build data connection: Connection refused\n");
        }
    }
    else
    {
        send_message(contfd, "530 Please login with USER and PASS\n");
    }
}

void dele(int contfd, char *buffer, int *userauth)
{
    if (userauth)
    {
        char command[BSIZE], filename[BSIZE], message[BSIZE];
        sscanf(buffer, "%s%s", command, filename);
        if (!remove(filename))
        {
            send_message(contfd, "250 DELE command successful\n");
        }
        else
        {
            sprintf(message, "550 %s: No such file or directory\n", filename);
            send_message(contfd, message);
        }
    }
    else
    {
        send_message(contfd, "530 Please login with USER and PASS\n");
    }
}

void quit(int contfd, int *connected, char *username)
{
    printf("221 %s Disconnected\n", username);
    send_message(contfd, "221 Goodbye.\n");
    close(contfd);
    *connected = FALSE;
}