#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int send_message(int contfd, char *message);

int print_error(char *message);

void user(int contfd, char *buffer, char *username);

void pass(int contfd, char *username, int *userauth);

void type(int contfd, char *buffer, char *mode);

void syst(int contfd);

void pasv(int contfd, int listenfd_data, char *ip, int *userauth);

void list(int contfd, int *datafd, int *userauth);

void retr(int contfd, int *datafd, char *buffer, char *mode, int *userauth);

void stor(int contfd, int *datafd, char *buffer, char *mode, int *userauth);

void dele(int contfd, char *buffer, int *userauth);

void quit(int contfd, int *connected, char *username);

#endif
