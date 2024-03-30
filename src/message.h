#pragma once 
#include <string>

enum HeaderType {
    Start,
    Stop,
    Name,
    Size,
    Data,
};

const char* getLabel(HeaderType type);
size_t getSplitPos(HeaderType type);
