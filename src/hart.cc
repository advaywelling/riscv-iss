#include "hart.h"
#include <iostream>

Hart::Hart() {
    pc = 0x0;
    write_word(0x0,  0x00000093);  // addi x1, x0, 0
    write_word(0x4,  0x00100113);  // addi x2, x0, 1
    write_word(0x8,  0x00600193);  // addi x3, x0, 6
    write_word(0xC,  0x002080B3);  // add  x1, x1, x2   <- loop start (0xC)
    write_word(0x10, 0x00110113);  // addi x2, x2, 1
    write_word(0x14, 0xFE311CE3);  // bne  x2, x3, -8   (back to 0xC)
}

uint32_t Hart::read_word(uint32_t addr) const {
    uint32_t read_data {};
    for (int i{}; i < 4; i++) {
        read_data |= mem[addr + i] << i*8;
    }
    return read_data;
}

void Hart::write_word(uint32_t addr, uint32_t write_data) {
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
                write_reg(rd, (int32_t)(int8_t)mem[addr]);
                break;
            case 0x1: // lh (sign-extend halfword)
                write_reg(rd, (int32_t)(int16_t)(mem[addr] | (mem[addr + 1] << 8)));
                break;
            case 0x2: // lw
                write_reg(rd, read_word(addr));
                break;
            case 0x4: // lbu (zero-extend byte)
                write_reg(rd, mem[addr]);
                break;
            case 0x5: // lhu (zero-extend halfword)
                write_reg(rd, mem[addr] | (mem[addr + 1] << 8));
                break;
        }
        break;
    }
    case 0x23: { // s type - stores
        uint32_t addr = regs[rs1] + imm_s;
        switch (funct3) {
            case 0x0: // sb
                mem[addr] = regs[rs2] & 0xFF;
                break;
            case 0x1: // sh
                mem[addr] = regs[rs2] & 0xFF;
                mem[addr + 1] = (regs[rs2] >> 8) & 0xFF;
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

    case 0x73: // system  - ecall / ebreak
        // placeholder, syscall handling will go here in the future. how far in the future? now that is a mystery
        break;
    case 0x0F: // no-op
        break;
    default:
        std::cerr << "Unknown opcode 0x" << std::hex << opcode << " at pc 0x" << curr_pc << "\n";
        break;
    }
}

int main() {
    Hart hart;
    for(int i{}; i < 20; i++) {
        hart.cycle();
    }
    hart.dump_regs();
}