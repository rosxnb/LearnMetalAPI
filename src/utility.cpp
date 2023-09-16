#include "utility.hpp"
#include <fstream>
#include <sstream>

std::string* Utility::read_source(const char* filepath)
{
    std::string* buffer = new std::string;

    std::ifstream fileHandler;
    fileHandler.exceptions( std::ifstream::badbit | std::ifstream::failbit );

    try 
    {
        fileHandler.open(filepath);
        std::stringstream sstream;
        sstream << fileHandler.rdbuf();
        buffer->assign( sstream.str() );
    }
    catch (std::ifstream::failure err)
    {
        __builtin_printf("Failed to load file: %s \n", filepath);
        __builtin_printf("%s \n\n", err.what());
    }

    return buffer;
}

