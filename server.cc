#include <netinet/in.h>
#include <vector>
#include <ctime>
#include <poll.h>
#include <cstring>
#include <iostream>

#include "err.h"

#define BUFFER_SIZE 1000

FILE* get_file_pointer(char *filename) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
        syserr("No such file: %s", filename);
    return fp;
}

class Client {
private:
    time_t start_time;
    sockaddr_in* address;
    static const unsigned TWO_MINUTES = 120;
public:
    Client(time_t start_time, sockaddr_in* client_address) : start_time(start_time), address(client_address) {}
    /**
     * Check if the client is equal to another.
     */
    bool equal(Client* other) {
        return address->sin_port == other->address->sin_port && address->sin_addr.s_addr == other->address->sin_addr.s_addr;
    }

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
    Client* source;
    static const unsigned long long secs_4243_year = 71697398400;

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
     */
    bool valid_datagram() {
        try {
            uint64_t timestamp = std::stoull(datagram);
            return timestamp < secs_4243_year;
        }
        catch (std::out_of_range& e) {
            return false;
        }
        catch (std::invalid_argument& e) {
            return false;
        }
    }

    /**
     * Appends content of the file to the datagram.
     * @filename name of the file
     */
    void append_file_content(FILE *fp) {
        size_t read_bytes = 0;

        read_bytes = fread(datagram + len - 1, sizeof(char), BUFFER_SIZE - len - 1, fp);
        fclose(fp);

        len += read_bytes;
        datagram[len] = '\0';
    }

    /**
    * Send udp datagram.
    * @client_address pointer to a structure with a client address
    * @sock socket
    */
    void send_udp(struct sockaddr_in* client_address, socklen_t snda_len, int sock) {
        int sflags = 0;
        ssize_t snd_len = sendto(sock, datagram, (size_t) len, sflags,
                                 (sockaddr *) client_address, snda_len);
        if (snd_len < 0 || size_t(snd_len) != len)
            syserr("error on sending datagram to client socket");
    }

    void set_source_client(Client *client) {
        source = client;
    }

    /**
     * Check if the given client is the source client
     */
    bool from_client(Client* client) {
        return source->equal(client);
    }

};

/**
 * Cyclic buffer storing datagrams.
 */
class DatagramCyclicBuffer {
private:
    static const int MAX_STORAGE = 4096;
    Datagram* datagram_buffer[MAX_STORAGE];
    int waiting_datagrams;
    int read_index, write_index;
public:
    DatagramCyclicBuffer() {
        waiting_datagrams = 0;
        read_index = 0;
        write_index = 0;
    }

    /**
     * Push datagram to the buffer. Overwrite the oldest.
     */
    void push_datagram(Datagram* datagram) {
        datagram_buffer[write_index] = datagram;
        write_index = (write_index + 1) % MAX_STORAGE;
        if (waiting_datagrams < 4096)
            waiting_datagrams++;
        else {
            read_index = (read_index + 1);
        }
    }

    /**
     * Take the oldest datagram. Return NULL if there is no datagram.
     */
    Datagram* take_datagram() {
        if (waiting_datagrams == 0)
            return NULL;
        waiting_datagrams--;
        return datagram_buffer[read_index++];
    }

};

class Server {
private:
    static const int MAX_CLIENTS = 42;
    static const int TIMEOUT_MILLISECS = 500;
    std::vector<Client*> clients_vec;
    char *filename;
    DatagramCyclicBuffer datagram_cyclic_buffer;
    pollfd* sock;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t snda_len, rcva_len;
    uint16_t port;
public:

    Server(char* filename, uint16_t port) : filename(filename), port(port) {
        sock = new pollfd;
        sock->fd = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
        if (sock->fd < 0)
            syserr("socket");

        server_address.sin_family = AF_INET; //IPv4
        server_address.sin_addr.s_addr = htonl(INADDR_ANY); //listening on all interfaces
        server_address.sin_port = htons(port);

        // bin the socket to a concrete address
        if (bind(sock->fd, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0)
            syserr("bind");

        snda_len = (socklen_t) sizeof(client_address);
    }

    /**
     * Add client to the vector of served clients.
     * @return success of adding a client.
     */
    bool add_client(Client* client) {
        /* if client already found then just update time */
        for (unsigned i = 0; i < clients_vec.size(); i++)
            if (clients_vec[i]->equal(client)) {
                clients_vec[i]->update_time(time(NULL));
                return true;
            }

        /* replace expired client */
        for (unsigned i = 0; i < clients_vec.size(); i++)
            if (!clients_vec[i]->is_alive()) {
                clients_vec[i] = client;
                return true;
            }

        /* push to the end if client limit not exceeded */
        if (clients_vec.size() < MAX_CLIENTS) {
            clients_vec.push_back(client);
            return true;
        }

        /* fail - too much active clients */
        return false;
    }

    /**
     * Add client to the vector of served clients.
     * @client_address pointer to the address of the client
     * @return pointer to the created client or NULL if client won't be served.
     */
    Client* add_client(sockaddr_in* client_address) {
        Client* client = new Client(time(NULL), client_address);
        return add_client(client) ? client : NULL;
    }

    /**
     * Receive udp datagram.
     */
    void receive_udp() {
        struct sockaddr_in* client_address = new(sockaddr_in);
        rcva_len = (socklen_t) sizeof(*client_address);
        int flags = 0; // we do not request anything special
        char* buffer = new char[BUFFER_SIZE];

        size_t len = (size_t) recvfrom(sock->fd, buffer, BUFFER_SIZE, flags,
                                       (sockaddr *) client_address, &rcva_len);

        Datagram* datagram = new Datagram(len, buffer);

        if (!datagram->valid_datagram()) {
            fprintf(stderr, "Wrong datagram from client %d port %d\n", client_address->sin_addr.s_addr, client_address->sin_port);
            return;
        }

        datagram->append_file_content(get_file_pointer(filename));

        datagram_cyclic_buffer.push_datagram(datagram);

        Client* client = add_client(client_address);

        datagram->set_source_client(client);

    }

    /**
     * Send udp datagram to all clients.
     */
    void send_to_all() {
        Datagram* datagram = datagram_cyclic_buffer.take_datagram();

        if (datagram != NULL)
            for (unsigned i = 0; i < clients_vec.size(); i++)
                if (clients_vec[i]->is_alive() && !datagram->from_client(clients_vec[i]))
                    datagram->send_udp(clients_vec[i]->get_address(), snda_len, sock->fd);

    }

    /**
     * @return if true then server listens datagrams, otherwise it broadcasts datagrams to clients.
     */
    bool listen() {
        sock->events = POLLIN;
        sock->revents = 0;
        return poll(sock, 1, TIMEOUT_MILLISECS) == 1;
    }
};


int main(int argc, char *argv[]) {

    if (argc < 3) {
        fatal("Usage: %s port filename... \n", argv[0]);
    }

    try {
        int port = std::stoi(argv[1]);
        if (port > UINT16_MAX || port < 0)
            syserr("Port too large");

        char* filename = argv[2];

        /* Just check if filename is valid. */
        (void) get_file_pointer(filename);

        Server server = Server(filename, (uint16_t) port);

    //********************************************


        while (true) {
            if (server.listen())
                server.receive_udp();
            else
                server.send_to_all();
        }
    }
    catch (std::out_of_range& e) {
        syserr("Port number is out of range");
    }
    catch (std::invalid_argument& e) {
        syserr("Port should be a number");
    }

}
