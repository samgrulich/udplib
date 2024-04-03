#pragma once 
#include <string>

enum HeaderType {
    Start,
    Stop,
    Name,
    Size,
    Data,
    Ack,
    Error,
};

// gets according label (c_str) for given Header (enum)
const char* getLabel(HeaderType type);
// gets start position of data (after header, ex. NAME= <- split position is 5)
size_t getSplitPos(HeaderType type);
