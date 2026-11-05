# 🗜️ Huffman Zipper (Pure C Archive Tool)

A lightweight CLI archiver written in **pure C**, implementing the classic **Huffman coding algorithm** for lossless data compression. This project was built to master low-level memory management, bitwise operations, custom binary file structures, and tree-based data structures.

---

### ✨ Features / Возможности

- **Lossless Compression:** Uses custom Huffman tree construction based on character frequency analysis.
- **Multi-File Archiving:** Capable of packing multiple files into a single consolidated archive `.hf`.
- **Custom Binary Format:** Implements a strict file structure header (`HF`), serialization of frequency tables, file size metadata, and variable-length encoded bitstreams.
- **Buffered I/O Management:** Reads data in optimal chunks (`8192 bytes`) to reduce disk overhead and maximize performance.
- **Cross-Platform CLI:** Fully controlled via UNIX-like flags (`-c`, `-x`, `-v`, `-h`).

---

### 🛠️ Technical Deep Dive & Architecture

The archiver introduces a strict custom binary layout for each compressed file inside the archive:
`[Header: "HF"] -> [Frequency Table] -> [Null Terminator] -> [File Size (uint64_t)] -> [Filename + \0] -> [Bitstream]`

- **Memory Optimization:** Dynamically built Huffman trees using minimal node popping queues with systematic memory freeing (`free_tree`) to prevent dangerous leaks.
- **Bit-Level Packing:** Uses an 8-bit buffer with shifts (`<<` and `|`) to pack variable-length Huffman codes into standard bytes.
- **POSIX Robustness:** Supports large files via 64-bit file offset definitions (`_FILE_OFFSET_BITS 64`).

---

### 🚀 Usage / Использование

#### Arguments / Флаги:
- `-h` — Show help screen / Вызов справки.
- `-c` — Compress files / Сжатие файлов в архив.
- `-x` — Extract archive / Распаковка архива.
- `-v` — Verbose mode (detailed logs) / Подробный вывод процессов.

#### Examples:
```bash
# Compress multiple files into one archive
# Сжатие нескольких файлов в один архив
./zipper -c file1.txt photo.bmp -o my_archive.hf

# Extract files from the archive with verbose logs
# Распаковка архива с подробным выводом логов
./zipper -x my_archive.hf -v
```

---

### 🚧 Future Prospects (Roadmap)
- Upgrade the algorithm into full **DEFLATE** compression by combining **LZ77** dictionary matching with Huffman encoding.
- Refactor structural global state logic to fully stabilize edge cases in streaming extractions.

---

### 🌋 License & Author

- **Author:** nsulti (Started: July 11, 2026 | Finished: July 25, 2026)
- **License:** **GNU GPLv3** — Code is free to analyze, modify, and distribute.

> *Note from the author:* Someday. Someday I shall return.
