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
//reads bytes from file, if number of read elements is wrong -> close file and throws
extern void        FileReadBuffer(void* buffer, std::size_t elemSize, std::size_t numElems, std::FILE* fi);
extern void        BadFile(std::FILE* fi);
extern std::string ReadLine(std::FILE* fi);

//reads formatted line from file, if errors occur -> closes file and throws
template<typename...Args>
void FileLineScanf(std::FILE* fi, Args&&... args)
{
    if(std::sscanf(ReadLine(fi).c_str(), args...) != sizeof...(args)-1)
        BadFile(fi);
}

}//namespace SafeRead

#endif // SAFEREAD_H
