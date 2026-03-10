# Memory-Efficient Versioned File Indexer

A command-line C++ program that processes large text files using a fixed-size buffer and builds a word-level index to support analytical queries.

## How to Compile

```bash
g++ -std=c++17 -O3 240608_mahi.cpp -o analyzer
```

## How to Run

### Word Count Query
Returns how many times a word appears in a file.
```bash
./analyzer --file dataset_v1.txt --version v1 --buffer 1024 --query word --word error
```

### Top-K Query
Displays the top K most frequent words in a file.
```bash
./analyzer --file dataset_v1.txt --version v1 --buffer 1024 --query top --top 10
```

### Difference Query
Shows the difference in frequency of a word between two file versions.
```bash
./analyzer --file1 dataset_v1.txt --version1 v1 --file2 dataset_v2.txt --version2 v2 --buffer 1024 --query diff --word error
```

## Command-Line Arguments

| Argument | Description |
|----------|-------------|
| `--file <path>` | Input file (word / top queries) |
| `--file1 <path>` | First input file (diff query) |
| `--file2 <path>` | Second input file (diff query) |
| `--version <name>` | Version name (word / top queries) |
| `--version1 <name>` | First version name (diff query) |
| `--version2 <name>` | Second version name (diff query) |
| `--buffer <kb>` | Buffer size in KB (256 to 1024) |
| `--query <type>` | Query type: `word`, `top`, or `diff` |
| `--word <token>` | Word to search (word / diff queries) |
| `--top <k>` | Number of top results (top query) |

## Design Overview

The program is built using four classes:

- **buffer_reader** — reads the file incrementally in fixed-size chunks so the entire file is never loaded into memory
- **tokenizer** — extracts words from each buffer chunk, handles words split across buffer boundaries, and is case-insensitive
- **version_index** — stores a separate word frequency map per version using an unordered_map
- **query_base / word_query / topk_query / diff_query** — abstract base class with derived query classes using inheritance and virtual functions

A function template `get_top_k<T>` sorts results by frequency with alphabetical tie-breaking.
