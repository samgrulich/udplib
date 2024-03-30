#pragma once 
#include <string>

enum HeaderType {
    Start,
    Stop,
    Name,
    Size,
    Data,
};

std::string getLabel(HeaderType type);
size_t getSplitPos(HeaderType type);
