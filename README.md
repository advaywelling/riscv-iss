# RISC-V ISS

A small RISC-V instruction set simulator (ISS) written in C++. It loads a
statically-linked ELF binary, then fetches, decodes, and executes one
instruction at a time until the program exits.

**Status:** passes the full RV32IM compliance suite — **50/50** of the
`rv32ui` + `rv32um` machine-mode (`-p-`) tests from
[riscv-tests](https://github.com/riscv-software-src/riscv-tests).

## Supported

- **RV32I** base integer instruction set
- **RV32M** multiply/divide extension
- Enough machine-mode plumbing to run the compliance harness:
  - CSR instructions (`csrrw/s/c` + immediate forms)
  - `mret`, and `mhartid` reads as 0
  - `ecall` (used both for syscalls and as the test pass/fail signal)
  - `fence` / `fence.i` as no-ops

### Not (yet) supported

- Traps / privilege modes / `mtvec` dispatch — the `ecall` exit is handled
  directly, so the `-v-` (virtual memory), `rv32mi`, and `rv32si` test suites
  won't run
- Virtual memory (Sv32) — no page tables / MMU
- Other extensions: F/D (float), A (atomics), C (compressed)

## Build

```bash
make
```

Produces the `riscv-iss` binary. Requires a C++17 compiler (`clang++` by default).

## Run

Point it at any statically-linked RISC-V ELF:

```bash
./riscv-iss path/to/program.elf
```

With no argument it falls back to `tests/multiply.elf`.

The process reports the outcome and sets its **exit status** accordingly:

| Output      | Exit status | Meaning                                              |
|-------------|-------------|------------------------------------------------------|
| `PASS`      | 0           | program exited via `ecall` with code 0               |
| `FAIL #N`   | 1           | program exited with a nonzero code (test #N failed)  |
| `TIMEOUT`   | 2           | ran `MAX_CYCLES` without exiting (likely infinite loop) |

## Testing against riscv-tests

The compliance tests live in a separate checkout of
[riscv-tests](https://github.com/riscv-software-src/riscv-tests). Build the
32-bit ISA tests (needs the `riscv64-unknown-elf` toolchain with rv32 multilib):

```bash
cd /path/to/riscv-tests/isa
make XLEN=32 rv32ui rv32um
```

This produces bare-metal ELFs named `rv32ui-p-add`, `rv32um-p-mul`, etc. Run
the whole set through the simulator and tally:

```bash
cd /path/to/RISC-V\ ISS
for t in /path/to/riscv-tests/isa/rv32u{i,m}-p-*; do
  case "$t" in *.dump) continue;; esac
  ./riscv-iss "$t"
done
```

### How pass/fail works

Each compliance test ends by writing a result code and calling `ecall` with
`a7 = 93` (exit). On **pass**, `a0 = 0`; on **fail**, `a0 = (failing_test_num << 1) | 1`.
The simulator captures `a0` as the exit code, so `main` reports PASS, or
FAIL with the failing sub-test number (`code >> 1`). The `.dump` disassembly
file next to each ELF is the easiest way to trace a specific failure.

## Layout

| File                | Purpose                                              |
|---------------------|------------------------------------------------------|
| `src/main.cc`       | entry point: loads the ELF, drives the run loop, reports the outcome |
| `src/hart.cc`       | the CPU core: fetch / decode / execute, CSRs, syscalls |
| `include/hart.h`    | `Hart` class — registers, memory, CSRs, PC           |
| `src/elf.cc`        | ELF parser: loads `PT_LOAD` segments, sets the entry PC |
| `include/elf.h`     | `load_elf` declaration                                |

## Notes

- Guest memory is a flat 1 MB array based at `0x8000_0000` (the address the
  test linker script uses). Every access subtracts that base to index the array.
- The run loop is capped at 10M instructions (`MAX_CYCLES`) so a runaway test
  can't hang.
