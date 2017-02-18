#ifndef COMMON_H_
#define COMMON_H_

#ifdef NDEBUG
    #define verify(expression) ((void)(expression))
#else
    #include <cassert>
    #define verify(expression) assert(expression)
#endif /* NDEBUG */

#include <string>
using STR = std::string;
using CSR = const STR&;

#endif /* COMMON_H_ */
