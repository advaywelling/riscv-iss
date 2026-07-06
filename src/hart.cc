#include <iostream>
#include <fstream>
#include <vector>
#include "hart.h"

Hart::Hart() {
    pc = 0x0;
}

bool Hart::load_binary(const std::string& filename, uint32_t addr) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        return false;
    }
    std::streamsize size = file.tellg();
    if (addr + size > MEM_SIZE) {
        std::cerr << "Error - binary too big for memory\n";
        return false;
    }
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(mem+addr), size);
    return true;
}

void Hart::load_segment(uint32_t addr, const uint8_t* data, uint32_t size) {
    addr -= MEM_BASE;
    for (uint32_t i{}; i < size; i++) {
        mem[addr + i] = data[i];
    }
}

uint32_t Hart::read_word(uint32_t addr) const {
    addr -= MEM_BASE;
    uint32_t read_data {};
    for (int i{}; i < 4; i++) {
        read_data |= mem[addr + i] << i*8;
    }
    return read_data;
}

void Hart::write_word(uint32_t addr, uint32_t write_data) {
    addr -= MEM_BASE;
    for (int i{}; i < 4; i++) {
        mem[addr + i] = (write_data >> i * 8) & 0xFF;
    }
}

void Hart::write_reg(uint32_t reg_num, uint32_t write_data) {
    if (reg_num != 0) regs[reg_num] = write_data;
}

void Hart::dump_regs() const {
    for (int i{}; i < 32; i++) {
        std::cout << "x" << std::dec << i << " = 0x" << std::hex << regs[i] << "\n"; 
    }
    std::cout << "pc = 0x" << std::hex << pc << "\n";
}

void Hart::cycle() {
    uint32_t instr = read_word(pc);

    uint32_t opcode = instr & 0x7F;
    uint32_t rd = (instr >> 7) & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x07;
    uint32_t rs1 = (instr >> 15) & 0x1F;
    uint32_t rs2 = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    int32_t imm_i = (int32_t)instr >> 20;
    int32_t imm_s = ((int32_t)(instr & 0xFE000000) >> 20) | ((instr >> 7) & 0x1F);
    int32_t imm_b = ((int32_t)(instr & 0x80000000) >> 19)
                  | ((instr & 0x7E000000) >> 20)
                  | ((instr & 0x00000F00) >> 7)
                  | ((instr & 0x00000080) << 4);
    int32_t imm_u = (int32_t)(instr & 0xFFFFF000);
    int32_t imm_j = ((int32_t)(instr & 0x80000000) >> 11)
                  | (instr & 0x000FF000)
                  | ((instr & 0x00100000) >> 9)
                  | ((instr & 0x7FE00000) >> 20);


    uint32_t curr_pc = pc;
    pc += 4;

    switch (opcode) {
    
    case 0x33: // r type -> register-register
        if (funct7 == 0x01) {
            switch(funct3) {
                case 0x0: // mul
                    write_reg(rd, regs[rs1] * regs[rs2]);
                    break;
                case 0x1: { // mul
                    int64_t result = (int64_t)(int32_t)regs[rs1] * (int64_t)(int32_t)regs[rs2];
                    write_reg(rd, (uint32_t)(result >> 32));
                    break;
                }
                case 0x2: { // mulhsu
                    int64_t result = (int64_t)(int32_t)regs[rs1] * (int64_t)(uint32_t)regs[rs2];
                    write_reg(rd, (uint32_t)(result >> 32));
                    break;
                }
                case 0x3: { // mulhu
                    int64_t result = (uint64_t)regs[rs1] * (uint64_t)regs[rs2];
                    write_reg(rd, (uint32_t)(result >> 32));
                    break;
                }
                case 0x4: { // div
                    int32_t dividend = (int32_t)regs[rs1];
                    int32_t divisor = (int32_t)regs[rs2];
                    if (divisor == 0) {
                        write_reg(rd, 0xFFFFFFFF);
                    } else if (dividend == INT32_MIN && divisor == -1) {
                        write_reg(rd, dividend);
                    } else {
                        write_reg(rd, dividend / divisor);
                    }
                    break;
                }
                case 0x5: { // divu
                    uint32_t dividend = regs[rs1];
                    uint32_t divisor = regs[rs2];
                    if (divisor == 0) {
                        write_reg(rd, 0xFFFFFFFF);
                    } else {
                        write_reg(rd, dividend / divisor);
                    }
                    break;
                }
                case 0x6: { // rem
                    int32_t dividend = (int32_t)regs[rs1];
                    int32_t divisor = (int32_t)regs[rs2];
                    if (divisor == 0) {
                        write_reg(rd, dividend);
                    } else if (dividend == INT32_MIN && divisor == -1) {
                        write_reg(rd, 0);
                    } else {
                        write_reg(rd, dividend % divisor);
                    }
                    break;
                }
                case 0x7: { // remu
                    uint32_t dividend = regs[rs1];
                    uint32_t divisor = regs[rs2];
                    if (divisor == 0) {
                        write_reg(rd, dividend);
                    } else {
                        write_reg(rd, dividend % divisor);
                    }
                    break;
                }
            }
        } else {
            switch (funct3) {
            case 0x0: // add+sub
                write_reg(rd, (funct7 == 0x20) ? regs[rs1] - regs[rs2] : regs[rs1] + regs[rs2]); 
                break; 
            case 0x1: // sll
                write_reg(rd, regs[rs1] << (regs[rs2] & 0x1F)); 
                break;
            case 0x2: // slt
                write_reg(rd, ((int32_t)regs[rs1] < (int32_t)regs[rs2]) ? 1 : 0); 
                break; 
            case 0x3: // sltu
                write_reg(rd, (regs[rs1] < regs[rs2]) ? 1 : 0); 
                break; 
            case 0x4: // xor
                write_reg(rd, regs[rs1] ^ regs[rs2]); 
                break; 
            case 0x5: // sra+srl
                write_reg(rd, (funct7 == 0x20) ? (uint32_t)((int32_t)regs[rs1] >> (regs[rs2] & 0x1F)) : regs[rs1] >> (regs[rs2] & 0x1F)); 
                break; 
            case 0x6: // or
                write_reg(rd, regs[rs1] | regs[rs2]); 
                break; 
            case 0x7: // and
                write_reg(rd, regs[rs1] & regs[rs2]); 
                break;  
            }
        }
        break;
    case 0x13: // i type -> register-immediate
        switch (funct3) {
            case 0x0: // addi
                write_reg(rd, regs[rs1] + imm_i);
                break;
            case 0x2: // slti
                write_reg(rd, ((int32_t)regs[rs1] < imm_i) ? 1 : 0);
                break;
            case 0x3: // sltiu
                write_reg(rd, (regs[rs1] < (uint32_t)imm_i) ? 1 : 0);
                break;
            case 0x4: // xori
                write_reg(rd, regs[rs1] ^ imm_i);
                break;
            case 0x6: // ori
                write_reg(rd, regs[rs1] | imm_i);
                break;
            case 0x7: // andi
                write_reg(rd, regs[rs1] & imm_i);
                break;
            case 0x1: // slli
                write_reg(rd, regs[rs1] << (rs2 & 0x1F));
                break;
            case 0x5: // srai + srli
                write_reg(rd, (funct7 == 0x20) ? (uint32_t)((int32_t)regs[rs1] >> (rs2 & 0x1F)) : regs[rs1] >> (rs2 & 0x1F));
                break;
        }
        break;
    
    case 0x03: { // i type - loads
        uint32_t addr = regs[rs1] + imm_i;
        switch (funct3) {
            case 0x0: // lb (sign-extend byte)
                write_reg(rd, (int32_t)(int8_t)read_byte(addr));
                break;
            case 0x1: // lh (sign-extend halfword)
                write_reg(rd, (int32_t)(int16_t)(read_byte(addr) | (read_byte(addr + 1) << 8)));
                break;
            case 0x2: // lw
                write_reg(rd, read_word(addr));
                break;
            case 0x4: // lbu (zero-extend byte)
                write_reg(rd, read_byte(addr));
                break;
            case 0x5: // lhu (zero-extend halfword)
                write_reg(rd, read_byte(addr) | (read_byte(addr + 1) << 8));
                break;
        }
        break;
    }
    case 0x23: { // s type - stores
        uint32_t addr = regs[rs1] + imm_s;
        switch (funct3) {
            case 0x0: // sb
                write_byte(addr, regs[rs2] & 0xFF);
                break;
            case 0x1: // sh
                write_byte(addr, regs[rs2] & 0xFF);
                write_byte(addr + 1, (regs[rs2] >> 8) & 0xFF);
                break;
            case 0x2: // sw
                write_word(addr, regs[rs2]);
                break;
        }
        break;
    }
    case 0x63: // b type - branches
        switch (funct3) {
            case 0x0: // beq
                if (regs[rs1] == regs[rs2]) pc = curr_pc + imm_b;
                break;
            case 0x1: // bne
                if (regs[rs1] != regs[rs2]) pc = curr_pc + imm_b;
                break;
            case 0x4: // blt
                if ((int32_t)regs[rs1] < (int32_t)regs[rs2]) pc = curr_pc + imm_b;
                break;
            case 0x5: // bge
                if ((int32_t)regs[rs1] >= (int32_t)regs[rs2]) pc = curr_pc + imm_b;
                break;
            case 0x6: // bltu
                if (regs[rs1] < regs[rs2]) pc = curr_pc + imm_b;
                break;
            case 0x7: // bgeu
                if (regs[rs1] >= regs[rs2]) pc = curr_pc + imm_b;
                break;
        }
        break;
    case 0x6F: // j type - jal
        write_reg(rd, pc);
        pc = curr_pc + imm_j;
        break;

    case 0x67: { // i type - jalr
        uint32_t target = (regs[rs1] + imm_i) & ~1u;
        write_reg(rd, pc);
        pc = target;
        break;
    }
    case 0x37: // u type - lui
        write_reg(rd, imm_u);
        break;
    case 0x17: // u type - auipc
        write_reg(rd, curr_pc + imm_u);
        break;

    case 0x73: { // syscall - csr/ecall/ebreak
        uint32_t csr_addr = (instr >> 20) & 0xFFF;
        switch (funct3) {
            case 0x0:
                if (instr == 0x00000073) {
                    handle_ecall();
                } else if (instr == 0x30200073) {
                    pc = read_csr(CSR_MEPC);
                }
                // ebreak not handled yet. will it ever be handled? who knows
                break;
            case 0x1: { // csrrw
                uint32_t o = read_csr(csr_addr);
                write_csr(csr_addr, regs[rs1]);
                write_reg(rd, o);
                break;
            }
            case 0x2: { // csrrs
                uint32_t o = read_csr(csr_addr);
                if (rs1) {
                    write_csr(csr_addr, o | regs[rs1]);
                }
                write_reg(rd, o);
                break;
            }
            case 0x3: { // csrrc
                uint32_t o = read_csr(csr_addr);
                if (rs1) {
                    write_csr(csr_addr, o & ~regs[rs1]);
                }
                write_reg(rd, o);
                break;
            }
            case 0x5: { // csrrwi
                uint32_t o = read_csr(csr_addr);
                write_csr(csr_addr, rs1);
                write_reg(rd, o);
                break;
            }
            case 0x6: { // csrrsi
                uint32_t o = read_csr(csr_addr);
                if (rs1) {
                    write_csr(csr_addr, o | rs1);
                }
                write_reg(rd, o);
                break;
            }
            case 0x7: { // csrrci
                uint32_t o = read_csr(csr_addr);
                if (rs1) {
                    write_csr(csr_addr, o & ~rs1);
                }
                write_reg(rd, o);
                break;
            }
        }
        break;
    }
    case 0x0F: // no-op
        break;
    default:
        std::cerr << "Unknown opcode 0x" << std::hex << opcode << " at pc 0x" << curr_pc << "\n";
        break;
    }
}

void Hart::handle_ecall() {
    uint32_t syscall_num = regs[17];
    
    switch (syscall_num) {
        case 93: { // exit
            running = false;
            exit_code = regs[10];
            break;
        }
        case 64: { // write to file
            uint32_t fd = regs[10]; // file descriptor
            uint32_t buff_addr = regs[11]; // output buffer addr
            uint32_t bytes = regs[12]; // num char to write

            for(uint32_t i{}; i < bytes; i++) {
                char c = (char)mem[buff_addr + i];
                if (fd == 1 || fd == 2) std::cout << c;
            }
            regs[10] = bytes;
            break;
        }
        default:
            std::cout << "Unhandled syscall: " << syscall_num << '\n';
            break;
    }
}

uint32_t Hart::read_csr(uint32_t addr) const {
    if (addr == CSR_MHARTID) return 0;  
    return csr[addr];
}

void Hart::write_csr(uint32_t addr, uint32_t val) {
    if (addr == CSR_MHARTID) return;    
    csr[addr] = val;
}
