#pragma once 
#include <string>

enum HeaderType {
    Start,
    Stop,
    Name,
    Size,
    Data,
};

std::string getLabel(HeaderType type) {
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
    }
    return "";
}

size_t getSplitPos(HeaderType type) {
    switch(type) {
        case Name: 
            return 4;
        case Size:
            return 4;
        default:
            return 0;
    }
    return 0;
}

