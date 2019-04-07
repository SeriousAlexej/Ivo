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
#include "saferead.h"

CSafeFile::CSafeFile(const std::string& path)
{
    m_file = std::fopen(path.c_str(), "rb");
}

CSafeFile::~CSafeFile()
{
    Close();
}

CSafeFile::operator bool() const
{
    return m_file != nullptr;
}

void CSafeFile::BadFile()
{
    Close();
    throw std::runtime_error("Error reading from file!");
}

std::string CSafeFile::ReadLine()
{
    std::string line = "";
    char tmp;
    while(1)
    {
        if(std::feof(m_file))
            break;
        size_t read = std::fread(&tmp, 1, 1, m_file);
        if(read != 1)
            BadFile();
        if(tmp == '\n')
            break;
        line += (tmp);
    }
    return line;
}

void CSafeFile::ReadBuffer(void* buffer, std::size_t elemSize, std::size_t numElems)
{
    SafetyCheck();

    if(std::fread(buffer, elemSize, numElems, m_file) != numElems)
        BadFile();
}

void CSafeFile::SafetyCheck()
{
    if(!m_file)
        throw std::runtime_error("File is not opened!");
}

void CSafeFile::Close()
{
    if(m_file)
        std::fclose(m_file);
    m_file = nullptr;
}
