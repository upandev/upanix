// tls_server.c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int main() {
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);

    // Load cert + key (PEM files)
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0)
        ERR_print_errors_fp(stderr);
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0)
        ERR_print_errors_fp(stderr);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8443);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, 1);

    printf("TLS Server listening on port 8443...\n");

    while (1) {
        int client = accept(sockfd, NULL, NULL);

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            char buffer[1024] = {0};
            SSL_read(ssl, buffer, sizeof(buffer));
            printf("Client says: %s\n", buffer);

            SSL_write(ssl, "Hello from TLS server!", 23);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}

