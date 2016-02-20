#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <iostream>

typedef unsigned char byte;
typedef std::vector<byte> ByteArray;

class NoOutput
{
public:
    template <typename T>
    NoOutput& operator<< (T) { return (*this); }
};

//daemon must no print in cerr. cout
//and for simplicity without logger
#ifndef __DAEMON__
    #define TRACE()  cerr  <<  __LINE__  << " "  << __FILE__ << " " << __FUNCTION__ << " "
#else
    #define TRACE() NoOutput()
    #define endl (1)
#endif


#endif // COMMON_H
