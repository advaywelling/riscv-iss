#include <fstream>
#include <iostream>
#include "hart.h"
#include "elf.h"

bool load_elf(Hart& hart, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error opening ELF: " << filename << "\n";
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    if (buffer[0] != 0x7F || buffer[1] != 'E' || buffer[2] != 'L' || buffer[3] != 'F') {
        std::cerr << "Not ELF file\n";
        return false; 
    }

    auto rd32 = [&](uint32_t offset) -> uint32_t {
        return buffer[offset] | (buffer[offset + 1] << 8) | (buffer[offset + 2] << 16) | (buffer[offset+3] << 24); // read 4 bytes from an offset in the file
    };
    auto rd16 = [&](uint32_t offset) -> uint32_t {
        return buffer[offset] | (buffer[offset + 1] << 8); // read 2 bytes from offset in file
    };

    uint32_t entry = rd32(24); // entry point
    uint32_t ph_offset = rd32(28); // program header table offset
    uint16_t ph_entry_size = rd16(42); // size of program header entry;
    uint16_t ph_num = rd16(44); // number of program headers

    for (int i{}; i < ph_num; i++) {
        uint32_t ph = ph_offset + i * ph_entry_size;
        uint32_t p_type = rd32(ph); // segment type
        uint32_t p_offset = rd32(ph + 4); // offset in file
        uint32_t p_addr = rd32(ph + 8); // where in mem it shall go
        uint32_t p_file_size = rd32(ph + 16); // file size - bytes

        if (p_type == 1) { // pt load i.e. loadable segment
            hart.load_segment(p_addr, &buffer[p_offset], p_file_size);
        }
    }
    hart.set_pc(entry);
    return true;
}