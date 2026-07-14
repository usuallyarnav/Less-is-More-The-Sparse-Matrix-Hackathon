# Less is More: The Sparse Matrix Hackathon

> A DSA course assignment that went a little too far — in the best way possible.

Instead of the expected array-of-triplets or linked list solution, this implementation uses a **bitmap index + dynamically allocated value vault** to represent a sparse matrix. The result: a 10×100 word-frequency matrix stored in roughly **185 bytes** instead of the naive **4,000 bytes** — a ~95% reduction.

---

## What Was Given vs What Was Built

The assignment came with starter code. Here's exactly what was pre-provided and what was written from scratch:

### Pre-provided (Starter Code)

**`sparse_utils.h`** — basic header with just two constants and the dense matrix declaration:
```c
#define MAX_LINES 10
#define MAX_WORDS 100
extern int matrix[MAX_LINES][MAX_WORDS]; // the naive 4,000-byte grid
```

**`sparse_utils.c`** — text data, dictionary helpers, and a `buildMatrix()` that populated the full dense matrix:
```c
// filled the entire 10×100 int matrix, zeros and all
void buildMatrix() { ... matrix[i][col]++; ... }
```

**`main.c`** — a `printAll()` that read directly from `matrix[i][j]` and a simple `main()` calling `buildMatrix()` then `printAll()`. No memory optimisation whatsoever.

### Built From Scratch

Everything related to compression is original work:

| What | Where |
|---|---|
| `bitmapSize` constant + `bitmap[125]` array | `sparse_utils.h` |
| `sparseValues` dynamic pointer + `nonZeroCount` | `sparse_utils.h` |
| `SET_BIT` / `GET_BIT` macros | `sparse_utils.h` |
| `compressMatrix()` — scans grid, allocates exact vault, fills bitmap, frees grid | `sparse_utils.c` |
| `getSparseValue(r, c)` — bitmap lookup + rank query for retrieval | `sparse_utils.c` |
| `freeSparseData()` — dynamic memory cleanup | `sparse_utils.c` |
| Reworked `buildMatrix()` — now uses `calloc` temp grid + calls `compressMatrix()` | `sparse_utils.c` |
| Reworked `main.c` — reads via `getSparseValue()` instead of `matrix[i][j]`, prints memory stats | `main.c` |

In short: the starter code handed you a naive dense matrix and said "make this better." Everything you see in the final solution that isn't the raw text data or the dictionary helpers (`findWord`, `addWord`) was designed and written as the solution.

---

## The Problem

In Natural Language Processing (and many other domains), matrices are naturally sparse. When you build a word-frequency matrix from sentences, each sentence only uses a handful of words out of the entire vocabulary. Storing every zero wastes enormous amounts of memory.

The naive approach for a 10-sentence corpus with 100 unique words:

```
10 rows × 100 columns × 4 bytes per int = 4,000 bytes
```

Most of that is zeros. The goal: store only what matters.

---

## The Approach: Bitmap + Dynamic Vault

This solution goes beyond the three methods suggested in the assignment (triplet array, linked list, hybrid row-wise). It borrows an idea from database indexing and compression — a **presence bitmap** paired with a **tightly packed value array**.

The design has two components:

### 1. The Bitmap (`unsigned char bitmap[125]`)

The full matrix has `10 × 100 = 1000` positions. Instead of storing a value at every position, a single bit is used to record whether that position is non-zero or not.

- `1000 bits ÷ 8 bits per byte = 125 bytes`
- Bit at position `(row * MAX_WORDS + col)` is `1` if that word appears in that sentence, `0` if not
- Two macros handle all bit manipulation cleanly:

```c
#define SET_BIT(map, pos) (map[(pos) / 8] |= (1 << ((pos) % 8)))
#define GET_BIT(map, pos) (map[(pos) / 8] & (1 << ((pos) % 8)))
```

### 2. The Value Vault (`unsigned char *sparseValues`)

Rather than a fixed-size array, the vault is allocated with **exactly** as many bytes as there are non-zero entries — no more, no less.

- Each frequency value fits in one `unsigned char` (1 byte), since no word appears more than 255 times in a sentence
- The vault is allocated via `malloc(nonZeroCount)` after scanning the temporary matrix
- Words in the same matrix are stored at `int` (4 bytes each) in the naive version; here each value costs just **1 byte**

### How Retrieval Works

To look up `matrix[row][col]`:

1. Compute the flat position: `pos = row * MAX_WORDS + col`
2. Check the bitmap at `pos` — if the bit is `0`, return `0` immediately (no search needed)
3. If the bit is `1`, count how many `1`-bits exist before `pos` in the bitmap
4. That count is the index into `sparseValues` — return `sparseValues[count]`

This is essentially a **rank query** on the bitmap, a technique used in succinct data structures.

---

## The Build Pipeline

The program never holds a full dense matrix in memory longer than necessary. The lifecycle is:

```
Text input
    ↓
calloc() a temporary 1D grid (4,000 bytes) — acts as a 2D matrix
    ↓
Tokenize sentences → populate word frequencies into the grid
    ↓
compressMatrix():
    → scan grid → count non-zero entries
    → malloc() sparseValues (exactly nonZeroCount bytes)
    → SET_BIT for every non-zero position
    → store frequency in sparseValues vault
    ↓
free() the temporary 4,000-byte grid  ← it's gone at this point
    ↓
Program runs entirely on ~185 bytes from here on
```

The temporary grid exists only during setup and is immediately destroyed after compression. The running program never touches 4,000 bytes again.

---

## `unsigned char` Instead of `int` — A Deliberate Choice

The naive matrix stores every frequency as an `int` — 4 bytes per value, even though the actual number stored is almost always `1` or `2`. That's up to 3 bytes of wasted space *per entry*, every single time.

The vault uses `unsigned char` instead, which is exactly 1 byte and can hold values from `0` to `255`. For a word-frequency matrix built from sentences, no word realistically appears more than 255 times in a single sentence — so nothing is lost, and you get a **4× size reduction on every stored value**:

```
Naive:  each frequency = int  = 4 bytes
Vault:  each frequency = char = 1 byte
```

This compounds with the bitmap savings. Not only are zeros eliminated entirely, but the non-zero values themselves are stored at a quarter of the original cost. The two optimizations together are what push the total from 4,000 bytes down to ~185.

It also fits naturally with the bitmap design — `unsigned char` is the same type used for the bitmap array itself, keeping the data types consistent and the code clean.

---

## Memory Breakdown

| Component | Size |
|---|---|
| Naive dense matrix | 4,000 bytes |
| Bitmap | 125 bytes |
| Value vault (exact, no waste) | ~60 bytes (depends on corpus) |
| **Total compressed** | **~185 bytes** |
| **Savings** | **~95%** |

The vault size is zero-waste by design — `malloc` is called only after the exact count of non-zero entries is known.

---

## File Structure

```
.
├── sparse_utils.h   # Definitions, macros, extern declarations — do not edit
├── sparse_utils.c   # Core engine: text parsing, matrix build, compress, retrieve
└── main.c           # Display logic and program entry point
```

### `sparse_utils.h`
Defines the constants, the bitmap and vault variables, and the two bit-manipulation macros. The `#ifndef` header guard prevents double-inclusion during compilation.

### `sparse_utils.c`
Contains all the logic:
- `buildMatrix()` — tokenizes text, builds temporary grid, calls `compressMatrix()`
- `compressMatrix()` — scans grid, allocates exact vault, fills bitmap, frees grid
- `getSparseValue(r, c)` — bitmap lookup + rank count for retrieval
- `freeSparseData()` — cleans up dynamic memory at program end

### `main.c`
Calls `buildMatrix()`, then `printAll()` to display the original text, vocabulary index, and the compressed matrix (zeros shown as `.` for readability), followed by the memory comparison.

---

## Build and Run

```bash
gcc main.c sparse_utils.c -o sparse_matrix_demo
./sparse_matrix_demo
```

Adding more sentences to the `lines` array in `sparse_utils.c` will automatically update the vocabulary and recompress — no other changes needed.

---

## Sample Output

```
--- Original Text ---
[0] data structures are fun and data is powerful
[1] algorithms and data structures are important
...

--- Vocabulary Index ---
0: data  1: structures  2: are  3: fun  ...

--- Compressed Sparse Matrix (Frequencies) ---
2 1 1 1 1 1 . . . . ...
. 1 1 . 1 . 1 . . . ...
...

Memory usage
Original size  : 4000 bytes
Bitmap         :  125 bytes
Vault          :   60 bytes (0 waste)
Total          :  185 bytes
```

---

## Why This Is Interesting

The assignment suggested three approaches: triplet arrays, linked lists, or a hybrid row-wise method. Each of those still stores the row and column index alongside every value — triplets alone cost `3 × 4 bytes = 12 bytes per non-zero entry`.

This bitmap approach eliminates the need to store row/column indices at all. The position of a bit in the bitmap *is* the index. The vault stores only raw values, and the rank of a set bit tells you exactly where in the vault to look. It is a form of **implicit indexing**.

The technique is related to succinct data structures used in compressed full-text search indexes and columnar databases — applied here to a simple NLP word-frequency problem from a DSA course.

---

## Concepts Demonstrated

- Bit manipulation (`|=`, `&`, `<<`, byte/bit addressing)
- Dynamic memory allocation (`malloc`, `calloc`, `free`) with zero over-allocation
- Modular C programming with header files and `extern`
- Sparse matrix theory and its real-world motivation in NLP
- Implicit indexing via bitmap rank queries
- Memory lifecycle management (build → compress → free → operate)

---

*Built as part of a Data Structures and Algorithms course. The assignment asked for less; this does more with less.*