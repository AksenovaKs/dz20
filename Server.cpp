#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

class Server {
private:
    int serverSocket_;
    vector<int> clientSockets_;
    vector<User> users_;

public:
    bool start(int port) {
        // Create server socket
        serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket_ < 0) {
            cerr << "Error opening server socket" << endl;
            return false;
        }

        // Set up server address
        struct sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(port);

        // Bind server socket to the specified address
        if (bind(serverSocket_, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            cerr << "Error binding server socket" << endl;
            return false;
        }

        // Start listening for incoming connections
        if (listen(serverSocket_, 5) < 0) {
            cerr << "Error listening for connections" << endl;
            return false;
        }

        cout << "Server started. Listening on port " << port << endl;

        return true;
    }

    void run() {
        while (true) {
            // Accept a new client connection
            struct sockaddr_in clientAddress;
            socklen_t clientAddressLength = sizeof(clientAddress);
            int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientAddressLength);
            if (clientSocket < 0) {
                cerr << "Error accepting client connection" << endl;
                continue;
            }

            // Add the client socket to the list
            clientSockets_.push_back(clientSocket);

            // Start a new thread to handle the client communication
            thread t(&Server::handleClient, this, clientSocket);
            t.detach();
        }
    }

    void handleClient(int clientSocket) {
        // Receive login credentials from the client
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead < 0) {
            cerr << "Error receiving login credentials from client" << endl;
            close(clientSocket);
            return;
        }

        string loginCredentials(buffer);

        // Parse login credentials
        size_t delimiterPos = loginCredentials.find(':');
        if (delimiterPos == string::npos) {
            cerr << "Invalid login credentials received from client" << endl;
            close(clientSocket);
            return;
        }

        string login = loginCredentials.substr(0, delimiterPos);
        string password = loginCredentials.substr(delimiterPos + 1);

        // Findпользователя с заданным логином и паролем
        auto it = find_if(users_.begin(), users_.end(), [&](const User& u){ return u.getLogin() == login && u.getPassword() == password; });

        if (it == users_.end()) {
            // Invalid login or password
            string response = "Invalid login or password";
            send(clientSocket, response.c_str(), response.length(), 0);
            close(clientSocket);
            return;
        }

        User& user = *it;

        // Successful login
        string response = "Login successful";
        send(clientSocket, response.c_str(), response.length(), 0);

        while (true) {
            // Receive a message from the client
            memset(buffer, 0, sizeof(buffer));
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead < 0) {
                cerr << "Error receiving message from client" << endl;
                break;
            }

            string message(buffer);

            // Check if the message is a command to exit
            if (message == "exit") {
                break;
            }

            // Broadcast the message to all other clients
            broadcastMessage(&user, message);
        }

        // Client disconnected
        cout << "Client disconnected: " << user.getName() << endl;
        close(clientSocket);

        // Remove the client socket from the list
        auto socketIt = find(clientSockets_.begin(), clientSockets_.end(), clientSocket);
        if (socketIt != clientSockets_.end()) {
            clientSockets_.erase(socketIt);
        }
    }

    void broadcastMessage(User* sender, const string& message) {
        for (int clientSocket : clientSockets_) {
            // Skip the sender client
            if (clientSocket == sender->getSocket()) {
                continue;
            }

            // Send the message to the client
            send(clientSocket, message.c_str(), message.length(), 0);
        }
    }
};

int main() {
    // Create the chat server
    Server server;

    // Add some sample users
    server.addUser("user1", "password1", "User 1");
    server.addUser("user2", "password2", "User 2");

    // Start the server on port 12345
    if (!server.start(12345)) {
        return 1;
    }

    // Run the server
    server.run();

    return 0;
}
