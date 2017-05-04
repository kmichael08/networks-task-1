#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <vector>
#include <ctime>

#include "err.h"
#include "utils.h"

// TODO how to define this value
#define BUFFER_SIZE 1000


class Client {
private:
    time_t start_time;
    sockaddr_in* address;
    //TODO redefine it to 120
    static const unsigned TWO_MINUTES = 10;
public:
    Client(time_t start_time, sockaddr_in* client_address) : start_time(start_time), address(client_address) {}
    /* TODO implement it */
    bool equal(Client* other) { return false; }
    void update_time(time_t new_time) { start_time = new_time; }
    bool is_alive() {
        return address != NULL && time(NULL) - start_time <= TWO_MINUTES;
    }
    sockaddr_in* get_address() { return address; }
};

class Datagram {
private:
    size_t len;
    char datagram[BUFFER_SIZE];
    /* TODO check this value out */
    static const long long secs_1717_year = -7983878400;
    static const long long secs_4243_year = 71697398400;

public:
    /**
     * @len length of the datagram
     * @buffer data in the datagram
     */
    Datagram(size_t len, char* buffer) : len(len) {
        strncpy(datagram, buffer, len);
        datagram[len] = '\0';
    }

    /**
     * Validate the datagram.
     * TODO full validation, types
     */
    bool valid_datagram() {
        int64_t timestamp = atoll(datagram);
        return timestamp >= secs_1717_year && timestamp < secs_4243_year;
    }

    /**
     * Appends content of the file to the datagram.
     * @filename name of the file
     */
    void append_file_content(char* filename) {
        size_t read_bytes = 0;
        FILE *fp;
        fp = fopen(filename, "r");
        if (fp == NULL)
            syserr("No such file: %s", filename);

        read_bytes = fread(datagram + len - 1, sizeof(char), BUFFER_SIZE - len - 1, fp);
        fclose(fp);

        len += read_bytes;
    }

    /**
    * Send udp datagram.
    * @client_address pointer to a structure with a client address
    * @sock socket
    */
    void send_udp(struct sockaddr_in* client_address, socklen_t snda_len, int sock) {
        datagram[len] = '\0';
        int sflags = 0;
        ssize_t snd_len = sendto(sock, datagram, (size_t) len, sflags,
                                 (sockaddr *) client_address, snda_len);
        if (snd_len != len)
            syserr("error on sending datagram to client socket");
    }

    char* datagram_to_send() {
        return datagram;
    }

    unsigned length() {
        return len;
    }

};

class Server {
private:
    static const int MAX_CLIENTS = 42;
    std::vector<Client*> clients_vec;
    char *filename;
    Datagram* datagram;
public:

    Server(char* filename) : filename(filename) {}

    /**
     * Add client to the vector of served client.
     * @return success of adding a client.
     */
    bool add_client(Client* client) {
        for (int i = 0; i < clients_vec.size(); i++)
            if (clients_vec[i]->equal(client)) {
                clients_vec[i]->update_time(time(NULL));
                return true;
            }

        for (int i = 0; i < clients_vec.size(); i++)
            if (!clients_vec[i]->is_alive()) {
                clients_vec[i] = client;
                return true;
            }

        if (clients_vec.size() < MAX_CLIENTS) {
            clients_vec.push_back(client);
            return true;
        }

        return false;
    }

    bool add_client(sockaddr_in* client_address) {
        return add_client(new Client(time(NULL), client_address));
    }

    /**
     * Receive udp datagram.
     * @sock socket
     */
    void receive_udp(socklen_t* rcva_len, int sock) {
        struct sockaddr_in* client_address = new(sockaddr_in);
        *rcva_len = (socklen_t) sizeof(*client_address);
        int flags = 0; // we do not request anything special
        char buffer[BUFFER_SIZE];

        size_t len = (size_t) recvfrom(sock, buffer, BUFFER_SIZE, flags,
                                       (sockaddr *) client_address, rcva_len);

        datagram = new Datagram(len, buffer);

        if (!datagram->valid_datagram()) {
            fprintf(stderr, "Wrong datagram from %d\n", client_address->sin_addr.s_addr);
            return;
        }

        datagram->append_file_content(filename);

        add_client(client_address);

        printf("read from socket: %zd bytes: %.*s\n", len, (int) len, buffer);
    }



    /**
     * Send udp datagram to all clients_vec.
     * @len length of message to send
     * @sock socket
     */
    void send_to_all(socklen_t snda_len, int sock) {
        for (int i = 0; i < clients_vec.size(); i++)
            if (clients_vec[i]->is_alive())
                datagram->send_udp(clients_vec[i]->get_address(), snda_len, sock);
    }

};

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fatal("Usage: %s port filename... \n", argv[0]);
    }

    if (!is_num(argv[1]))
        fatal("Port should be a number");

    uint16_t port = atoi(argv[1]);
    char* filename = argv[2];

    printf("%d %s\n", port, filename);

    Server server = Server(filename);

    //********************************************

    int sock;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t snda_len, rcva_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    if (sock < 0)
        syserr("socket");

    server_address.sin_family = AF_INET; //IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //listening on all interfaces
    server_address.sin_port = htons(port);

    // bin the socket to a concrete address
    if (bind(sock, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0)
        syserr("bind");

    snda_len = (socklen_t) sizeof(client_address);

    //********************************************


    while (1) {
        server.receive_udp(&rcva_len, sock);
        server.send_to_all(snda_len, sock);
    }



    return 0;
}
