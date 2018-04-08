/*
    Ivo - a free software for unfolding 3D models and papercrafting
    Copyright (C) 2015-2018 Oleksii Sierov (seriousalexej@gmail.com)
	
    Ivo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ivo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ivo.  If not, see <http://www.gnu.org/licenses/>.
*/
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

template<typename... Args>
constexpr int argcount(Args&&... args) { return sizeof...(args); }

#define FIRST_ARG(a, ...) a
#define REST_ARGS(a, ...) __VA_ARGS__

#define FLINESCANF_IMPL___(num, f, ...)\
do { if(std::sscanf(SafeRead::ReadLine(f).c_str(), __VA_ARGS__) != num) SafeRead::BadFile(f); } while(false)

//reads formatted line from file, if errors occur -> closes file and throws
#define SAFE_FLSCANF(...) FLINESCANF_IMPL___(argcount(__VA_ARGS__)-2, FIRST_ARG(__VA_ARGS__), REST_ARGS(__VA_ARGS__))

//reads bytes from file, if number of read elements is wrong -> close file and throws
#define SAFE_FREAD(dst_buf, elem_sz, num_elems, file_stream) \
do { if(std::fread(dst_buf, elem_sz, num_elems, file_stream) != (std::size_t)num_elems) SafeRead::BadFile(file_stream); } while(false)

#endif // SAFEREAD_H
