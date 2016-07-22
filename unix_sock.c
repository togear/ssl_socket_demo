/*
 * =====================================================================================
 *
 *       Filename:  unix_sock.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年07月19日 10时19分16秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yong.huang (gk), yong.huang@ngaa.com.cn
 *   Organization:  
 *
 * =====================================================================================
 */
#include "unix_sock.h"

void send_fd(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = { 0 };
    char buf[CMSG_SPACE(sizeof(fd))];
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = "ABC", .iov_len = 3 };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    *((int *) CMSG_DATA(cmsg)) = fd;

    msg.msg_controllen = cmsg->cmsg_len;

    if (sendmsg(socket, &msg, 0) < 0)
        printf("Failed to send message %s\n",strerror(errno));
    else
        printf("Success to send message fd=[%d] through sock[%d]\n",fd,socket);
}

int receive_fd(int socket)  // receive fd from socket
{
    struct msghdr msg = {0};

    char m_buffer[256];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char c_buffer[256];
    msg.msg_control = c_buffer;
    msg.msg_controllen = sizeof(c_buffer);

    if (recvmsg(socket, &msg, 0) < 0)
        printf("Failed to receive message %s\n",strerror(errno));
    else
        printf("Success to reveive message through sock[%d]\n",socket);

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);

    unsigned char * data = CMSG_DATA(cmsg);

//    printf("About to extract fd\n");
    int fd = *((int*) data);
    printf("Extracted fd %d\n", fd);

    return fd;
}
