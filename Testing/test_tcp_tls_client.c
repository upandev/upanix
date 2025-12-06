// tls_client.c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int main() {
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);

    // Disable certificate verification (optional)
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(8443);
    //inet_pton(AF_INET, "192.168.50.76", &serv.sin_addr);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    connect(sockfd, (struct sockaddr*)&serv, sizeof(serv));

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        SSL_write(ssl, "Hello from TLS client!", 23);

        char reply[1024] = {0};
        SSL_read(ssl, reply, sizeof(reply));
        printf("Server replied: %s\n", reply);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);

    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}

