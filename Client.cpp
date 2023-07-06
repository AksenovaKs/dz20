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

class Client {
private:
    int clientSocket_;
    User* user_;

public:
    bool connect(const string& serverIP, int serverPort, const string& login, const string& password) {
        // Create client socket
        clientSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket_ < 0) {
            cerr << "Error opening client socket" << endl;
            return false;
        }

        // Set up server address
        struct sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);

        // Convert server IP address from stringto binary form
        if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddress.sin_addr)) <= 0) {
            cerr << "Invalid server IP address" << endl;
            return false;
        }

        // Connect to the server
        if (connect(clientSocket_, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            cerr << "Error connecting to server" << endl;
            return false;
        }

        // Send login credentials to the server
        string loginCredentials = login + ":" + password;
        send(clientSocket_, loginCredentials.c_str(), loginCredentials.length(), 0);

        // Receive response from the server
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket_, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead < 0) {
            cerr << "Error receiving response from server" << endl;
            close(clientSocket_);
            return false;
        }

        string response(buffer);

        if (response == "Invalid login or password") {
            cerr << "Invalid login or password" << endl;
            close(clientSocket_);
            return false;
        }

        cout << "Login successful" << endl;

        // Create the user object
        user_ = new User(login, password, "My Name");

        // Start a new thread to handle incoming messages
        thread t(&Client::receiveMessages, this);
        t.detach();

        return true;
    }

    void sendChatMessage(const string& message) {
        // Send the message to the server
        send(clientSocket_, message.c_str(), message.length(), 0);
    }

    void receiveMessages() {
        while (true) {
            // Receive a message from the server
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            int bytesRead = recv(clientSocket_, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead < 0) {
                cerr << "Error receiving message from server" << endl;
                break;
            }

            string message(buffer);

            // Print the received message
            cout << "Received message: " << message << endl;
        }

        // Server disconnected
        cout << "Server disconnected" << endl;
        close(clientSocket_);
    }
};

int main() {
    // Create the chat client
    Client client;

    // Connect to the server
    if (!client.connect("127.0.0.1", 12345, "user1", "password1")) {
        return 1;
    }

    // Send chat messages
    client.sendChatMessage("Hello, Server!");
    client.sendChatMessage("How are you?");

    // Wait for user input to exit
    cout << "Press Enter to exit" << endl;
    cin.ignore();

    return 0;
}
