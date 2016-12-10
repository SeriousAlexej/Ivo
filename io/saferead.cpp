#include <string>
#include <cstdio>
#include <stdexcept>

void BadFile(std::FILE* fi)
{
    if(fi)
        std::fclose(fi);
    throw std::logic_error("Error reading from file!");
}

std::string ReadLine(FILE* fi)
{
    std::string line = "";
    char tmp;
    while(1)
    {
        size_t read = std::fread(&tmp, 1, 1, fi);
        if(read != 1)
            BadFile(fi);
        if(tmp == '\n')
            break;
        line += (tmp);
    }
    return line;
}
