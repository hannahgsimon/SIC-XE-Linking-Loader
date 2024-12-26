# SIC/XE Linking Loader

## Overview
- SIC/XE is a hypothetical architecture introduced in *System Software: An Introduction to Systems Programming*, by Leland Beck to explain the concepts of assemblers, compilers, and operating systems [1; 2].
- This program implements a SIC/XE linking loader in C, designed to manage a symbol table and memory buffers for processing object code. It simulates key components of assembly processes by adding symbols to the symbol table, processing text and modification records, and filling memory buffers.
- The loader operates in two passes:
  - Pass 1: Constructs the External Symbol Table (`ESTAB`).
  - Pass 2: Processes object code and loads it into a simulated memory buffer (`MEM`).

## Features
- Object File Parsing: Reads object files, each containing one object program. The total object code size across all files may not exceed 512 bytes.  
- Symbol Table Management: Tracks control sections, symbol names, addresses, and lengths (`ESTAB`). Prevents duplicates and allows retrieval of symbol addresses by name or number.
- Optimized Object Program Support: Supports object programs with a modified format where external symbols in a control section are assigned reference numbers, replacing symbol names in Modification records. This approach minimizes redundant searches of `ESTAB` during control section loading by looking up each external reference symbol only once per control section. It also simplifies code modification retrieval by indexing into an array.
- Memory Buffer Management: Simulates a 1024-byte memory buffer to store content for various memory addresses (`MEM`). Handles text and modification records by converting hexadecimal values into memory buffer contents.
- Error Reporting: Identifies and reports errors for duplicate or undefined external symbols.
- File Output: Generates tables for symbol and memory buffer contents for easy visualization.

## Setup & Usage
1. Ensure the following prerequisites are installed:
    - C Compiler (e.g., GCC, MSVC)
    - Command-line interface to run the assembler program
    - Standard C libraries: `stdio.h`, `stdlib.h`, `string.h`, `errno.h`, `ctype.h`
2. Compile the source file into an executable:  
    ```bash
    gcc siclink.c -o siclink
    ```
3. Run the program with at least 3 object file names and the starting address (in hexadecimal) passed as command-line arguments.
   Sample input and output files are included in the `Standard_SIC_XE sample_io` and `Enhanced_SIC_XE sample_io` directories for reference.
    - `Standard_SIC_XE sample_io` argument:
      ```bash
      ./siclink PROGA.txt PROGB.txt PROGC.txt 4000
      ```
    - `Enhanced_SIC_XE sample_io` argument:
      ```bash
      ./siclink EC_PROGA.txt EC_PROGB.txt EC_PROGC.txt 4000
      ```

## Output  
The program generates an output file named **`outfile.txt`**, containing:  
1. **ESTAB (External Symbol Table)**
2. **MEM (Memory Contents After Linking)**  
   - Four groups of four bytes per row, separated by two spaces
   - Memory address of the first byte in each row is displayed on the left
   - Gaps in memory are displayed as `........`
3. **Errors:** If a duplicate or undefined external symbol is encountered, the error is written to the console and the program terminates.

## References
[1] Beck, L. L. (1997). *System Software: An Introduction to Systems Programming* (3rd ed.). Addison-Wesley.  
[2] Wikimedia Foundation. (2024, December 16). *Simplified Instructional Computer*. Wikipedia. https://en.wikipedia.org/wiki/Simplified_Instructional_Computer

## Authors
This project was collaboratively developed by Hannah G. Simon and Charlie Strickland.
