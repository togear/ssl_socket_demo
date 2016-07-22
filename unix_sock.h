/*
 * =====================================================================================
 *
 *       Filename:  unix_sock.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年07月19日 10时11分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yong.huang (gk), yong.huang@ngaa.com.cn
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef UNIX_SOCK_H
#define UNIX_SOCK_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define CLIENT_SOCK_FILE "client.sock"
#define SERVER_SOCK_FILE "server.sock"

#define MAX_LISTEN_NUM 100

void send_fd(int socket, int fd);  // send fd by socket
int receive_fd(int socket);  // receive fd from socket

#endif
