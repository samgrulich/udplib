#pragma once 
#include <cstdint>
#include <cstdlib>
#include <string>

#include "socket.h"


class Pipe {
private:
    int socket_;
    int fromlen_;
    struct sockaddr_in local_;
    struct sockaddr_in dest_;
    struct sockaddr_in from_;
    int32_t packetId_ = -1;
private:
    // sends bytes array to the remote
    long sendBytes(const char* bytes, int len);
    /** Recevies the incoming byte array
     *  and writes it to the buffer 
     *  (!!the buffer must be initialized at or greater than BUFFERS_LEN size)
     *  @param buffer(out) buffer for writing of incoming byte array (!must be initialized)
     *  @return bytes received
     */
    long recvBytes(char* buffer);
public:
    /** create new pipe
     *  @param remote_address the ipv4 address of remote host
     *  @param local_port the port at which this program listens
     *  @param remote_port the port at which the host listens
     */ 
    Pipe(const char* remote_address, const int local_port, const int remote_port);
    ~Pipe();
    // sends bytes array to the remote
    long sendBytesCRC(const char* bytes, int len);
    // sends bytes array to the remote
    long sendBytesCRC(const std::string& message);
    /** Recevies the incoming byte array
     *  and writes it to the buffer 
     *  (!!the buffer must be initialized at or greater than BUFFERS_LEN size)
     *  @param buffer(out) buffer for writing of incoming byte array (!must be initialized)
     *  @return bytes received or -1 socketerr, -2 iderr, -3 crcerr
     */
    long recvBytesCRC(char* buffer);
    // sets socket timeout to 1s
    void set_timeout();
    // sends string to the remote
    long send(const std::string& message);
    long send(const char* bytes, int len);
    /**
     * Blocks until a packet arrives, then 
     * sends acknowledgement, or error message in this 
     * case waits for another packet.
     * @param buffer(out) must be initialized of size at least BUFFERS_LEN
     * @return bytes received (without null char)
     */
    long recv(char* buffer);
    void incrementPacketId();
};
