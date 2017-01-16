#ifndef SAFEREAD_H
#define SAFEREAD_H
#include <cstdio>
#include <string>

namespace SafeRead
{
extern void        BadFile(std::FILE* fi);
extern std::string ReadLine(std::FILE* fi);
}

#define FLINESCANF_IMPL___(num, f, ...)\
do { if(std::sscanf(SafeRead::ReadLine(f).c_str(), __VA_ARGS__) != num) SafeRead::BadFile(f); } while(false)

#define GET_ARGCOUNT_IMPL___(_F, _S, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, NAME, ...) NAME

#define GET_ARGCOUNT(...)\
GET_ARGCOUNT_IMPL___(__VA_ARGS__, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

//reads formatted line from file, if errors occur -> closes file and throws
#define SAFE_FLSCANF(...) FLINESCANF_IMPL___(GET_ARGCOUNT(__VA_ARGS__), __VA_ARGS__)

//reads bytes from file, if number of read elements is wrong -> close file and throws
#define SAFE_FREAD(dst_buf, elem_sz, num_elems, file_stream) \
do { if(std::fread(dst_buf, elem_sz, num_elems, file_stream) != (std::size_t)num_elems) SafeRead::BadFile(file_stream); } while(false)

#endif // SAFEREAD_H
