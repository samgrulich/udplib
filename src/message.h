#pragma once 
#include <string>

// has to fit into one byte (0-255)
enum HeaderType {
    Ack = 0,
    Error = 1,
    FileAck = 2,
    FileError = 3,
    Start = 4,
    Stop = 5,
    Name = 6,
    Size = 7,
    Data = 8,
    Hash = 9,
    End = 10,
    MissingAck = 11,
    MissingPacket = 12,
};

