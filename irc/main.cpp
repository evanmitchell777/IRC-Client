#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <netdb.h>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string nickname;
std::string hostname;
std::string realname;
std::string channel;
std::string server;
std::string port;

int main() {
    // Read configuration from JSON file
    std::ifstream file("./config.json");
    json jsonData;
    file >> jsonData;
    file.close();

    // Assign values from JSON data to variables
    nickname = jsonData["nickname"];
    hostname = jsonData["hostname"];
    realname = jsonData["realname"];
    channel = jsonData["channel"];
    server = jsonData["server"];
    port = jsonData["port"];

    // Initialize OpenSSL
    SSL_library_init();
    SSL_CTX* sslContext = SSL_CTX_new(SSLv23_client_method());
    if (!sslContext) {
        std::cerr << "SSL context creation failed" << std::endl;
        return 1;
    }

    // Get server address information
    struct addrinfo hints, *serverInfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    int status = getaddrinfo(server.c_str(), port.c_str(), &hints, &serverInfo);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return 1;
    }

    // Create socket
    int clientSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    if (clientSocket == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // Connect to server
    if (connect(clientSocket, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
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

    std::string nickCommand = "NICK " + nickname + "\r\n";
    SSL_write(ssl, nickCommand.c_str(), nickCommand.size());

    std::string userCommand = "USER " + nickname + " " + hostname + " " + hostname + " :" + realname + "\r\n";
    SSL_write(ssl, userCommand.c_str(), userCommand.size());

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

            if (strstr(buffer, "NOTICE * :*** You have not registered")) {
                // Register the nickname
                std::string registerCommand = "PRIVMSG NickServ :REGISTER MyPassword MyEmailAddress\r\n";
                SSL_write(ssl, registerCommand.c_str(), registerCommand.size());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Check if identified and then join the channel
            std::string join_msg = "JOIN " + channel + "\r\n";
            SSL_write(ssl, join_msg.c_str(), join_msg.size());

            std::cout << ": ";
            std::string message;
            std::getline(std::cin, message);

            std::string privmsgCommand = "PRIVMSG " + channel + " :" + message + "\r\n";
            SSL_write(ssl, privmsgCommand.c_str(), privmsgCommand.size());
        }
    }

    // Cleanup
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(sslContext);
    close(clientSocket);

    return 0;
}

