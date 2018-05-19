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

class CSafeFile
{
public:
    CSafeFile(const std::string& path);
    CSafeFile(const CSafeFile&) = delete;
    CSafeFile(CSafeFile&&) = delete;
    ~CSafeFile();

    CSafeFile& operator=(const CSafeFile&) = delete;
    CSafeFile& operator=(CSafeFile&&) = delete;
    operator bool() const;

    template<typename...Args>
    void        LineScanf(Args&&... args);
    void        ReadBuffer(void* buffer, std::size_t elemSize, std::size_t numElems);
    void        SafetyCheck();
    std::string ReadLine();

private:
    void BadFile();
    void Close();

    std::FILE* m_file = nullptr;
};

//--------------------------------------------------------
template<typename...Args>
void CSafeFile::LineScanf(Args&&... args)
{
    SafetyCheck();
    if(std::sscanf(ReadLine().c_str(), args...) != sizeof...(args)-1)
        BadFile();
}

#endif // SAFEREAD_H
