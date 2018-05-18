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
#include <string>
#include <cstdio>
#include <stdexcept>

namespace SafeRead
{

void BadFile(std::FILE* fi)
{
    if(fi)
        std::fclose(fi);
    throw std::logic_error("Error reading from file!");
}

std::string ReadLine(std::FILE* fi)
{
    std::string line = "";
    char tmp;
    while(1)
    {
        if(std::feof(fi))
            break;
        size_t read = std::fread(&tmp, 1, 1, fi);
        if(read != 1)
            BadFile(fi);
        if(tmp == '\n')
            break;
        line += (tmp);
    }
    return line;
}

void FileReadBuffer(void* buffer, std::size_t elemSize, std::size_t numElems, std::FILE* fi)
{
    if(std::fread(buffer, elemSize, numElems, fi) != numElems)
        BadFile(fi);
}

}//namespace SafeRead
