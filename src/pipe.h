#pragma once 
#include <string>

#include "socket.h"


class Pipe {
private:
    int socket_;
    int fromlen_;
    struct sockaddr_in local_;
    struct sockaddr_in dest_;
    struct sockaddr_in from_;
public:
    /** create new pipe
     *  @param remote_address the ipv4 address of remote host
     *  @param local_port the port at which this program listens
     *  @param remote_port the port at which the host listens
     */ 
    Pipe(const char* remote_address, const int local_port, const int remote_port);
    ~Pipe();
    // sends string to the remote
    size_t send(const std::string& message);
    // sends bytes array to the remote
    size_t sendBytes(const char* bytes, int len);
    /** Recevies the incoming byte array
     *  and writes it to the buffer 
     *  (!!the buffer must be initialized at or greater than BUFFERS_LEN size)
     *  @param buffer(out) buffer for writing of incoming byte array (!must be initialized)
     *  @return bytes received
     */
    size_t recv(char* buffer);
};
