/*
 *FileName  :   ssl_server.c
 *Author    :   JiangInk
 *Version   :   V0.1
 *Date      :   2015.03.10
*/

/*****************************************************************************/
/*** ssl_server.c                                                          ***/
/***                                                                       ***/
/*** Demonstrate an SSL server.                                            ***/
/*****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "ssl_common.h"
#include "unix_sock.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

#define TEST_UNIX_SOCK_PASSING  1
//#undef TEST_UNIX_SOCK_PASSING


/*---------------------------------------------------------------------*/
/*--- open_listener - create server socket                          ---*/
/*---------------------------------------------------------------------*/
int open_listener(int port)
{
    int sd;
    struct sockaddr_in addr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 10) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }
    return sd;
}

/*---------------------------------------------------------------------*/
/*--- init_server_ctx - initialize SSL server  and create contexts  ---*/
/*---------------------------------------------------------------------*/
SSL_CTX* init_server_ctx(void)
{   
    SSL_METHOD *method;
    SSL_CTX *ctx;

    SSL_library_init();                 /* init algorithms library */
    OpenSSL_add_all_algorithms();       /* load & register all cryptos, etc. */
    SSL_load_error_strings();           /* load all error messages */
    //method = SSLv23_server_method();  /* create new server-method instance */
    method = TLSv1_server_method();
    ctx = SSL_CTX_new(method);          /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

/*---------------------------------------------------------------------*/
/*--- verify_callback - SSL_CTX_set_verify callback function.       ---*/
/*---------------------------------------------------------------------*/
int verify_callback(int ok, X509_STORE_CTX *store)
{
    char data[256];
    if (ok)
    {
        fprintf(stderr, "verify_callback\n{\n");
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        int  depth = X509_STORE_CTX_get_error_depth(store);
        int  err = X509_STORE_CTX_get_error(store);

        fprintf(stderr, "certificate at depth: %i\n", depth);
        memset(data, 0, sizeof(data));
        X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
        fprintf(stderr, "issuer = %s\n", data);
        X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
        fprintf(stderr, "subject = %s\n", data);
        fprintf(stderr, "error status:  %i:%s\n}\n", err, X509_verify_cert_error_string(err));
    }
    return ok;
}

/*---------------------------------------------------------------------*/
/*--- load_certificates - load from files.                          ---*/
/*---------------------------------------------------------------------*/
void load_certificates(SSL_CTX* ctx, char* CaFile, char* CertFile, char* KeyFile)
{
    #if 1
    /* set maximum depth for the certificate chain */
    //SSL_CTX_set_verify_depth(ctx, 1);
    /* set voluntary certification mode*/
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    /* set mandatory certification mode*/
    //SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);
    /* load CA certificate file */
    if (SSL_CTX_load_verify_locations(ctx, CaFile, NULL) <=0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    #endif
	/* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set server private key password */ 
    SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)PRIKEY_PASSWD);
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    /* set SSL cipher type */
    SSL_CTX_set_cipher_list(ctx, ALGO_TYPE);
}

/*---------------------------------------------------------------------*/
/*--- show_certs_info - print out certificates.                     ---*/
/*---------------------------------------------------------------------*/
void show_certs_info(SSL* ssl)
{
    X509 *cert;
    char *line;

    /* Get connect use algorithm type */
    //printf("SSL connection using %s\n", SSL_get_cipher(ssl));
    /* Get certificates (if available) */
    cert = SSL_get_peer_certificate(ssl);
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}

/*---------------------------------------------------------------------*/
/*--- server_handler - SSL servlet                                  ---*/
/*---------------------------------------------------------------------*/
void server_handler(SSL* ssl)   /* Serve the connection -- threadable */
{
    char buf[1024];
    char reply[1280];
    int sd, bytes;

    if (FAIL == SSL_accept(ssl))                    /* do SSL-protocol accept */
    {
        ERR_print_errors_fp(stderr);   
    }
    else
    {
        show_certs_info(ssl);                       /* get any certificates */
        bytes = SSL_read(ssl, buf, sizeof(buf)-1);  /* get request */
        if (FAIL == bytes)
        {
            ERR_print_errors_fp(stderr);
        }
        buf[bytes] = '\0';
        printf("Client msg: \"%s\"\n", buf);
        snprintf(reply, sizeof(reply), SERVERHTML, buf);
        bytes = SSL_write(ssl, reply, strlen(reply));   /* send reply */
        if (FAIL == bytes)
        {
            ERR_print_errors_fp(stderr);
        }
    }
    sd = SSL_get_fd(ssl);               /* get socket connection */
    //SSL_shutdown(ssl);                /* shutdown SSL link */
    SSL_free(ssl);                      /* release SSL state */
    close(sd);                          /* close connection */
}


#ifdef TEST_UNIX_SOCK_PASSING

/*---------------------------------------------------------------------*/
/*--- create a new unix socket to passing fd                        ---*/
/*---------------------------------------------------------------------*/

int create_unix_sock(void) {

    int fd;
    if ((fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
//    if ((fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, CLIENT_SOCK_FILE);
    unlink(CLIENT_SOCK_FILE);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }
    return fd;
} 

/*-----------------------------------------------------------------------------------*/
/*--- always connect a new connection between two processes by unix domain socket ---*/
/*------------------------------------------------------------------------------------*/

int connect_unix_domain_socket(int fd) {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));  
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_SOCK_FILE);  
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) { 
         perror("connect");   
         return -1;
    }
    return 1;
}

/*---------------------------------------------------------------------*/
/*--- test for https ssl fd paasing                                ---*/
/*---------------------------------------------------------------------*/

void test_for_fd_passing(int sock, int fd) {

    send_fd(sock, fd);

}
#endif

/*---------------------------------------------------------------------*/
/*--- main - create SSL socket server.                              ---*/
/*---------------------------------------------------------------------*/
int main(int count, char *strings[])
{
    SSL_CTX *ctx;
    int server;
    char *portnum;

    if ( count != 2 )
    {
        printf("Usage: %s <portnum>\n", strings[0]);
        exit(0);
    }
    portnum = strings[1];
    ctx = init_server_ctx();                                        /* initialize SSL */
    load_certificates(ctx, ROOTCERTF, SERVER_CERT, SERVER_KEYF);    /* load certs */
    server = open_listener(atoi(portnum));                          /* create server socket */

    int sock = create_unix_sock();
    printf("create_unix_sock = %d \n",sock);
    if(sock < 0)
    {
        printf("create unix sock error \n");
        return -1;
    }
    connect_unix_domain_socket(sock);

    while (1)
    {   
        struct sockaddr_in addr;
        memset(&addr, '\0', sizeof(struct sockaddr_in));
        socklen_t len = sizeof(addr);
        /* accept connection as usual */
        int client = accept(server, (struct sockaddr*)&addr, &len);
        if(client < 0)
        {
            printf("accept failure %s\n",strerror(errno));
            break;
        }
        printf("Connection: %s:%d\n",
               	inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        printf("accept client fd = %d \n",client);
#ifdef TEST_UNIX_SOCK_PASSING

        test_for_fd_passing(sock,client);
#else
        SSL *ssl;
        ssl = SSL_new(ctx);             /* get new SSL state with context */
        SSL_set_fd(ssl, client);        /* set connection socket to SSL state */
        server_handler(ssl);            /* service connection */
#endif
    }
    close(server);                      /* close server socket */
    SSL_CTX_free(ctx);                  /* release context */

    return 0;
}
