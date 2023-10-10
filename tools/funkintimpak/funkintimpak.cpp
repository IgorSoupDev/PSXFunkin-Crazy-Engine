/*
 * funkintimconv by Regan "CuckyDev" Green.
 * Converts image files to TIM files for the Friday Night Funkin' PSX port.
 * C++ version made by SoupDev.
*/

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"

typedef union
{
	struct
	{
        uint16_t r : 5; // 5 bytes (0-31)
        uint16_t g : 5; // 5 bytes (0-31)
        uint16_t b : 5; // 5 bytes (0-31)
        uint16_t i : 1; // 1 bytes (0-1)
    } c;

    uint16_t v;
} RGBI;

int main(int argc, char* argv[]) 
{
    // Read parameters
    if (argc < 3) 
    {
        std::cout << "usage: funkintimpak out.tim in.png" << std::endl;
        return 0;
    }

    const char* outpath = argv[1];
    const char* inpath = argv[2];

    char* txtpath = new char[strlen(inpath) + 5];
    if (txtpath == nullptr) 
    {
        std::cerr << "Failed to allocate txt path" << std::endl;
        return 1;
    }
    sprintf(txtpath, "%s.txt", inpath);

    std::ifstream txtfp(txtpath);
    delete[] txtpath;

    if (!txtfp.is_open()) 
    {
        std::cerr << "Failed to open " << inpath << ".txt" << std::endl;
        return 1;
    }

    int tex_x, tex_y, pal_x, pal_y, bpp;

    if (!(txtfp >> tex_x >> tex_y >> pal_x >> pal_y >> bpp))
    {
        std::cerr << "Failed to read parameters from " << inpath << ".txt" << std::endl;
        return 1;
    }

    // Validate parameters
    int max_colour, width_shift;
    switch (bpp) 
    {
        case 4:
            max_colour = 16;
            width_shift = 2;
            break;
        case 8:
            max_colour = 256;
            width_shift = 1;
            break;
        default:
            std::cerr << "Invalid bpp " << bpp << std::endl;
            return 1;
    }

    // Read image contents
    int tex_width, tex_height;
    stbi_uc* tex_data = stbi_load(inpath, &tex_width, &tex_height, nullptr, 4);
    if (tex_data == nullptr) 
    {
        std::cerr << "Failed to read texture data from " << inpath << std::endl;
        return 1;
    }

    if (tex_width & ((1 << width_shift) - 1)) 
    {
        std::cerr << "Width " << tex_width << " can't properly be represented with bpp of " << bpp << std::endl;
        stbi_image_free(tex_data);
        return 1;
    }

    // Convert image
    RGBI pal[256];
    int pals_i = 0;
    memset(pal, 0, sizeof(pal));

    size_t tex_size = ((tex_width << 1) >> width_shift) * tex_height;
    uint8_t* tex = new uint8_t[tex_size];
    if (tex == nullptr) 
    {
        std::cerr << "Failed to allocate texture buffer" << std::endl;
        stbi_image_free(tex_data);
        return 1;
    }

    stbi_uc* tex_datap = tex_data;
    uint8_t* texp = tex;
    for (int i = tex_width * tex_height; i > 0; i--, tex_datap += 4) 
    {
        // Get palette representation
        RGBI rep;

        // If the alpha transparency is greater than 128, we will draw the pixel, otherwise it's transparent.
        if (tex_datap[3] > 128) 
        {
            // Convert RGB 888 to RGB 555
            rep.c.r = tex_datap[0] >> 3;
            rep.c.g = tex_datap[1] >> 3;
            rep.c.b = tex_datap[2] >> 3;
            rep.c.i = 1;
        } else 
        {
            rep.c.r = 0;
            rep.c.g = 0;
            rep.c.b = 0;
            rep.c.i = 0;
        }

        // Get palette index
        int pal_i = 0;
        while (1) 
        {
            if (pal_i >= max_colour) 
            {
                std::cerr << "Image has more than " << max_colour << " colours" << std::endl;
                delete[] tex;
                stbi_image_free(tex_data);
                return 1;
            }
            if (pal_i >= pals_i) 
            {
                pal[pal_i].v = rep.v;
                pals_i++;
                break;
            }
            if (pal[pal_i].v == rep.v)
                break;
            pal_i++;
        }

        // Write pixel
        switch (bpp) 
        {
            case 4:
                if (i & 1)
                    *texp++ = *texp | (pal_i << 4);
                else
                    *texp = pal_i;
                break;
            case 8:
                *texp++ = pal_i;
                break;
        }
    }
    stbi_image_free(tex_data);

    // Write output
    std::ofstream outfp(outpath, std::ios::binary);
    if (!outfp.is_open())
    {
        std::cerr << "Failed to open " << outpath << std::endl;
        delete[] tex;
        return 1;
    }

    //Header
    outfp.put(0x10);
    outfp.put(0);
    outfp.put(0);
    outfp.put(0);
    switch (bpp)
    {
        case 4:
            outfp.put(0x08);
            break;
        case 8:
            outfp.put(0x09);
            break;
    }
    outfp.put(0);
    outfp.put(0);
    outfp.put(0);

    //CLUT
    pals_i = max_colour;
    uint32_t clut_length = 12 + 2 * pals_i;
    outfp.put(clut_length);
    outfp.put((clut_length >> 8));
    outfp.put((clut_length >> 16));
    outfp.put((clut_length >> 24));
    outfp.put(pal_x);
    outfp.put((pal_x >> 8));
    outfp.put(pal_y);
    outfp.put((pal_y >> 8));
    outfp.put(pals_i);
    outfp.put((pals_i >> 8));
    outfp.put(1);
    outfp.put(0);

    // Write palette data (CLUT)
    outfp.write(reinterpret_cast<const char*>(pal), pals_i * 2);

    //Texture
    uint32_t tex_length = 12 + (((tex_width << 1) >> width_shift) * tex_height);
    outfp.put(tex_length);
    outfp.put((tex_length >> 8));
    outfp.put((tex_length >> 16));
    outfp.put((tex_length >> 24));
    outfp.put(tex_x);
    outfp.put((tex_x >> 8));
    outfp.put(tex_y);
    outfp.put((tex_y >> 8));
    outfp.put((tex_width >> width_shift));
    outfp.put(((tex_width >> width_shift) >> 8));
    outfp.put(tex_height);
    outfp.put((tex_height >> 8));

    // Write texture data
    outfp.write(reinterpret_cast<const char*>(tex), ((tex_width << 1) >> width_shift) * tex_height);

    outfp.close();

    delete[] tex;

    return 0;
}
