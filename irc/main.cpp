#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>

int main() {
    // Initialize OpenSSL
    SSL_library_init();
    SSL_CTX* sslContext = SSL_CTX_new(SSLv23_client_method());
    if (!sslContext) {
        std::cerr << "SSL context creation failed" << std::endl;
        return 1;
    }

    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // Specify server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(6697);
    serverAddress.sin_addr.s_addr = inet_addr("108.181.30.70");

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Connection to server failed" << std::endl;
        return 1;
    }

    // Attach SSL to socket
    SSL* ssl = SSL_new(sslContext);
    if (!ssl) {
        std::cerr << "SSL creation failed" << std::endl;
        return 1;
    }
    SSL_set_fd(ssl, clientSocket);
    if (SSL_connect(ssl) <= 0) {
        std::cerr << "SSL connection failed" << std::endl;
        return 1;
    }



//send nickname
//    std::string nick = "NICK NICKNAME|cpp\r\n";
//    send(clientSocket, nick.c_str(),nick.size(), 0);
/////////////////////////////////////////////////////



//send hostname 
//std::string hostname = "USER your_username hostname servername :hostnameyou\r\n";
//send(clientSocket,hostname.c_str(), hostname.size(),0);

  char buffer[1024];
    while (true) {
        int bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            std::cerr << "Error receiving message" << std::endl;
            break; // Exit loop on error
        } else if (bytes_read == 0) {
            std::cerr << "Connection closed" << std::endl;
            break; // Exit loop on connection close
        } else {
            buffer[bytes_read] = '\0'; // Null-terminate the received data
            std::cout << buffer << std::endl;
        }
    }



    // Cleanup
      SSL_shutdown(ssl);
      SSL_free(ssl);
      SSL_CTX_free(sslContext);
      close(clientSocket);

    return 0;
}