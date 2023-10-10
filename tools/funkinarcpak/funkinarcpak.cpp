/*
 * funkinarcpak by Regan "CuckyDev" Green.
 * Packs files into a single archive for the Friday Night Funkin' PSX port.
 * C++ version by SoupDev.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

void Write16(std::ofstream& fp, uint16_t x) 
{
    fp.put(x >> 0);
    fp.put(x >> 8);
}

void Write32(std::ofstream& fp, uint32_t x) 
{
    fp.put(x >> 0);
    fp.put(x >> 8);
    fp.put(x >> 16);
    fp.put(x >> 24);
}

int main(int argc, char *argv[]) 
{
    // Ensure the correct parameters have been provided
    if (argc < 3) 
    {
        std::cout << "usage: funkinarcpak out ..." << std::endl;
        return 0;
    }

    // Open the output file
    std::ofstream out(argv[1], std::ios::binary);
    if (!out.is_open()) 
    {
        std::cerr << "Failed to open " << argv[1] << std::endl;
        return 1;
    }

    // Define a vector for the directory
    struct Pkg_Directory 
    {
        char name[12];
        uint32_t pos;
        uint32_t size;
        uint8_t* data;
    };

    std::vector<Pkg_Directory> dir(argc - 2);

    // Read files and fill the directory
    for (int i = 2; i < argc; i++) 
    {
        Pkg_Directory& dir_entry = dir[i - 2];

        // Open the file
        std::ifstream in(argv[i], std::ios::binary);
        if (!in.is_open()) 
        {
            std::cerr << "Failed to open " << argv[i] << std::endl;
            for (int j = 2; j < i; j++) 
            {
                delete[] dir[j - 2].data;
            }
            return 1;
        }

        // Read the file
        in.seekg(0, std::ios::end);
        dir_entry.size = static_cast<uint32_t>(in.tellg());
        dir_entry.data = new uint8_t[dir_entry.size];
        in.seekg(0, std::ios::beg);
        in.read(reinterpret_cast<char*>(dir_entry.data), dir_entry.size);
        in.close();
    }

    // Set directory positions
    dir[0].pos = 16 * (argc - 2);
    for (size_t i = 1; i < dir.size(); i++) 
    {
        dir[i].pos = (dir[i - 1].pos + dir[i - 1].size + 0xF) & ~0xF;
    }

    // Write the directory
    for (size_t i = 0; i < dir.size(); i++) 
    {
        // Trim the path
        char* path = strrchr(argv[i + 2], '/') + 1;
      
        // Write the file name
        if (strlen(path) > 12) 
        {
            std::cout << "Warning: " << path << " name is longer than 12 characters!" << std::endl;
            out.write(path, 12);
        } 
        else 
        {
            out.write(path, strlen(path));
            for (size_t j = strlen(path); j < 12; j++) 
            {
                out.put('\0');
            }
        }

        Write32(out, dir[i].pos);
    }

    // Write file data
    for (size_t i = 0; i < dir.size(); i++) 
    {
        out.seekp(dir[i].pos);
        out.write(reinterpret_cast<char*>(dir[i].data), dir[i].size);
        delete[] dir[i].data;
    }

    return 0;
}

