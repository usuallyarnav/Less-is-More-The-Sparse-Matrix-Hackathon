# Less is More — A Bitmap-Indexed Sparse Matrix in C

> **4,000 bytes → 185 bytes.** No row indices. No column indices. No pointers.
> Just a bitmap and a byte-packed vault.

A university DSA assignment asked for a sparse matrix using triplet arrays or linked lists. This is neither. It stores a 10×100 NLP word-frequency matrix using a **presence bitmap plus a rank-indexed value vault** — the same core idea that powers Apache Arrow, Apache Parquet, and Roaring Bitmap.

---

## The Headline

Let **K** = number of non-zero entries, **R×C** = matrix dimensions.

| Method | Formula | This corpus (R=10, C=33, K=60) |
|---|---|---|
| Dense `int` matrix (naive) | `4 × R × C` | **4,000 bytes** |
| Linked list nodes | `~16K` | **~960 bytes** |
| COO triplets, `int` | `12K` | **720 bytes** |
| CSR, `int` | `4K + 4K + 4R` | **520 bytes** |
| COO triplets, `char` | `3K` | **180 bytes** |
| **Bitmap + vault (this)** | **`R×C/8 + K`** | **102 bytes** ✅ |

Every other sparse format stores *where* a value lives. This one doesn't. The **position of a set bit is the coordinate** — so the row and column indices cost literally zero bytes.

> ⚠️ **Honest note:** the shipped code reports **185 bytes**, not 102, because the bitmap is currently provisioned for `MAX_WORDS = 100` while the corpus only produces 33 unique words. Sizing the bitmap to the actual vocabulary (`10 × 33 / 8 = 42 bytes`) drops it to **102 bytes**. See [Known Limitations](#known-limitations). I'd rather report the number the program actually prints than a flattering one.

---

## Why This Beats COO — The Density Rule

COO (Coordinate List) is the standard sparse format. Even in its most aggressive form — every field crushed down to a single `unsigned char` — it must store **three bytes per non-zero entry**: row, column, and value. That floor is unavoidable, because COO has no way to know *where* a value belongs without writing the coordinates down.

This implementation stores **one byte per entry** (the value alone) plus a **fixed** `R×C/8` bitmap.

So the crossover is a clean inequality:

```
     R×C/8 + K   <   3K
⟺    R×C/8       <   2K
⟺    K / (R×C)   >   1/16
```

> ### **This approach beats COO whenever matrix density exceeds 6.25%.**

That's the whole result. Below 6.25% density the bitmap's fixed cost dominates and COO wins. Above it, the bitmap amortizes and this pulls away — permanently, because it scales at **1 byte per entry** while COO scales at **3**.

**This corpus sits at 18.2% density** (60 non-zeros out of 330 cells), comfortably past the threshold.

And the gap compounds as the corpus grows — the bitmap is a one-time cost, the vault is not:

| K (non-zeros) | COO (`char`) | Bitmap + Vault | Saving |
|---|---|---|---|
| 60 | 180 B | 102 B | 43% |
| 200 | 600 B | 242 B | 60% |
| 500 | 1,500 B | 542 B | 64% |
| 1,000 | 3,000 B | 1,042 B | 65% |

Asymptotically this approaches a **3× advantage over the best possible COO**, and a **12× advantage over textbook COO**.

---

## How It Works

Two structures, that's it.

### 1. The Bitmap — `unsigned char bitmap[125]`

The matrix has `10 × 100 = 1000` cells. Instead of a value per cell, store **one bit** per cell: `1` if non-zero, `0` if not.

```
1000 cells ÷ 8 bits per byte = 125 bytes
```

Bit `i` corresponds to flat position `i = row × MAX_WORDS + col`. Two macros do all the work:

```c
#define SET_BIT(map, pos) (map[(pos) / 8] |= (1 << ((pos) % 8)))
#define GET_BIT(map, pos) (map[(pos) / 8] &  (1 << ((pos) % 8)))
```

`pos / 8` finds the byte, `pos % 8` finds the bit inside it.

### 2. The Vault — `unsigned char *sparseValues`

A dense, gap-free array holding **only the non-zero values**, in row-major order. Allocated with `malloc(nonZeroCount)` *after* the exact count is known — so it is provably zero-waste. Not one spare byte.

### Retrieval — a Rank Query

To read `matrix[r][c]`:

```c
int pos = r * MAX_WORDS + c;

if (!GET_BIT(bitmap, pos)) return 0;   // ← O(1) rejection, no search

int count = 0;
for (int i = 0; i < pos; i++)          // ← rank: how many 1s came before?
    if (GET_BIT(bitmap, i)) count++;

return sparseValues[count];            // ← that rank IS the vault index
```

The number of set bits before position `pos` is exactly how many values were packed into the vault before this one. So **rank = vault index**. The coordinates were never stored; they were *recomputed* from the bitmap's geometry.

This is [Jacobson's rank](https://en.wikipedia.org/wiki/Succinct_data_structure) (1989), the foundation of succinct data structures.

---

## `unsigned char` Over `int` — 4× on Every Value

The naive matrix spends 4 bytes on an `int` to store a frequency that is almost always `1` or `2`. The vault uses `unsigned char` — 1 byte, range 0–255 — which is more than sufficient for word counts within a single sentence.

```
naive : 4 bytes/value
vault : 1 byte/value      → 4× reduction, before the bitmap even helps
```

This stacks with the bitmap: zeros are eliminated *and* the survivors cost a quarter as much. It also keeps types consistent — the bitmap is `unsigned char` too.

---

## The Build Pipeline

The dense matrix exists only as scaffolding, and is demolished before the program does any real work.

```
  text corpus
       │
       ▼
  calloc()  ─────────►  temp grid, 4,000 bytes (zero-filled)
       │
       ▼
  tokenize + count frequencies into grid
       │
       ▼
  compressMatrix()
       ├─ scan grid → count non-zeros → K
       ├─ malloc(K) → vault sized exactly, zero waste
       ├─ SET_BIT for every non-zero position
       └─ copy each value into the vault
       │
       ▼
  free(tempMatrix)  ◄── the 4,000 bytes are GONE
       │
       ▼
  program now runs on 185 bytes, permanently
```

`freeSparseData()` releases the vault at exit. No leaks.

---

## Prior Art — What This Actually Is

I want to be straight about this: **I did not invent this.** I arrived at it independently while trying to beat the assignment's suggested approaches, and only afterwards found out it's a well-established production technique. That's arguably the more interesting outcome.

The bitmap-plus-packed-values layout is, in essence:

- **Apache Arrow** — validity bitmaps sit beside a packed values buffer; nulls cost 1 bit, exactly as here
- **Apache Parquet** — definition levels encode presence separately from data
- **Roaring Bitmap** — bitmap containers for dense chunks, used by Lucene, Druid, InfluxDB
- **Succinct data structures** — Jacobson's rank/select, RRR encoding, wavelet trees
- **Bitmap indexes** — standard in column-store databases for low-cardinality columns

So: not novel. But the fact that a from-scratch attempt to outperform COO converges on the exact design used by columnar databases suggests the idea is a genuine local optimum, not a trick.

---

## Known Limitations

Being honest about the tradeoffs, because they're real:

**1. Lookups are O(n), not O(1).**
`getSparseValue()` performs a linear scan to compute rank, so a single read costs O(pos) and printing the whole matrix is O((R×C)²). Dense arrays are O(1). This is the price paid for storing no indices — and it is **solvable**: real succinct structures precompute cumulative popcounts every 64 bits ("rank superblocks"), restoring **O(1) lookup** at ~3% extra space. That's the natural next step.

**2. The bitmap is over-provisioned.**
It's sized for `MAX_WORDS = 100`, but the corpus yields 33 unique words. 67 columns of bitmap are pure padding. Sizing it to `lineCount × wordCount` after tokenization takes it from **125 → 42 bytes**, and total from **185 → 102 bytes**.

**3. Values are capped at 255.**
`unsigned char` overflows above 255 occurrences of one word in one sentence. Fine for sentence-level NLP; not fine in general.

**4. Below 6.25% density, COO wins.** See the density rule above. This is not a universal victory, and shouldn't be sold as one.

**5. It's immutable once built.** Inserting a new non-zero would require shifting the entire vault. Linked lists handle dynamic updates far better — that's their actual advantage, and this design gives it up.

---

## Build & Run

```bash
gcc main.c sparse_utils.c -o sparse_matrix_demo
./sparse_matrix_demo
```

Add sentences to the `lines` array in `sparse_utils.c` — the vocabulary and compression update automatically.

### Output

```
--- Vocabulary Index ---
0: data  1: structures  2: are  3: fun  4: and  5: is  6: powerful  ...  32: hand

--- Compressed Sparse Matrix (Frequencies) ---
2 1 1 1 1 1 1 . . . . . . . . . . . . . . . . . . . . . . . . . .
1 1 1 . 1 . . 1 1 . . . . . . . . . . . . . . . . . . . . . . . .
1 1 . . 1 . . . . 1 1 1 1 . . . . . . . . . . . . . . . . . . . .
3 . . . . . . . . . . . . 1 . . . . . . . . 1 1 . . . . . . . . .
...

                 Memory usage
Original size     : 4000 bytes
Bitmap            :  125 bytes
Vault             :   60 bytes (0 waste)
Total Memory Used :  185 bytes
```

Every `.` is a byte that was never allocated.

---

## What Was Given vs What Was Built

The assignment shipped starter code. To be transparent about authorship:

**Provided:** the text corpus, the `findWord()` / `addWord()` vocabulary helpers, a `buildMatrix()` that filled a dense `int matrix[10][100]`, and a `printAll()` that read from it directly. No compression of any kind.

**Written from scratch:**

| Component | File |
|---|---|
| `bitmap[]` array + `bitmapSize` | `sparse_utils.h` |
| `SET_BIT` / `GET_BIT` macros | `sparse_utils.h` |
| `sparseValues` vault pointer + `nonZeroCount` | `sparse_utils.h` |
| `compressMatrix()` — scan, allocate, pack, free | `sparse_utils.c` |
| `getSparseValue()` — rank-query retrieval | `sparse_utils.c` |
| `freeSparseData()` — leak-free teardown | `sparse_utils.c` |
| `buildMatrix()` rewritten around `calloc`/`free` | `sparse_utils.c` |
| `main.c` rewritten to read via `getSparseValue()` + report memory | `main.c` |

The dense matrix declaration was **deleted entirely**. It no longer exists in the final program.

---

## Files

```
├── sparse_utils.h   # constants, extern decls, bit macros
├── sparse_utils.c   # tokenizer, compressor, rank retrieval, cleanup
└── main.c           # display + memory reporting
```

---

## Concepts

Bit manipulation · succinct data structures · rank queries · dynamic memory (`malloc`/`calloc`/`free`) with zero over-allocation · sparse matrix formats (COO/CSR) · asymptotic space analysis · modular C with `extern` linkage

---

<sub>Built for a Data Structures & Algorithms course. The brief said "store fewer zeros." This stores none of them, and none of the coordinates either.</sub>