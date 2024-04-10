#include "message.h"

const char* getLabel(HeaderType type) {
    switch (type) {
        case Start:
            return "START";
        case Stop:
            return "STOP";
        case Name:
            return "NAME";
        case Size:
            return "SIZE";
        case Data:
            return "DATA";
        case Hash:
            return "HASH";
        case Ack:
            return "ACK";
        case Error:
            return "ERROR";
        case FileAck:
            return "FILEACK";
        case FileError:
            return "FILEERROR";
        default:
            return "";
    }
    return "";
}

size_t getSplitPos(HeaderType type) {
    switch(type) {
        case Name: 
            return 5;
        case Size:
            return 5;
        case Ack:
            return 3;
        default:
            return 5;
    }
    return 0;
}

