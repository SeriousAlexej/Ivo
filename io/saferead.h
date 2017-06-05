#ifndef SAFEREAD_H
#define SAFEREAD_H
#include <cstdio>
#include <string>
#include <tuple>

namespace SafeRead
{
extern void        BadFile(std::FILE* fi);
extern std::string ReadLine(std::FILE* fi);
}

#define FIRST_ARG(a, ...) a
#define REST_ARGS(a, ...) __VA_ARGS__

#define FLINESCANF_IMPL___(num, f, ...)\
do { if(std::sscanf(SafeRead::ReadLine(f).c_str(), __VA_ARGS__) != num) SafeRead::BadFile(f); } while(false)

#define GET_ARGCOUNT(...)\
std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value

//reads formatted line from file, if errors occur -> closes file and throws
#define SAFE_FLSCANF(...) FLINESCANF_IMPL___(GET_ARGCOUNT(__VA_ARGS__)-2, FIRST_ARG(__VA_ARGS__), REST_ARGS(__VA_ARGS__))

//reads bytes from file, if number of read elements is wrong -> close file and throws
#define SAFE_FREAD(dst_buf, elem_sz, num_elems, file_stream) \
do { if(std::fread(dst_buf, elem_sz, num_elems, file_stream) != (std::size_t)num_elems) SafeRead::BadFile(file_stream); } while(false)

#endif // SAFEREAD_H
