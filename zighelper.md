# ZigHelper

`zighelper` searches either:
- Zig `cimport.zig` cache files (C API signatures), or
- Zig source trees (project code or Zig stdlib).

## Build

```powershell
zig build
```

Binary output:

```text
.\zig-out\bin\zighelper.exe
```

## Usage

```text
zighelper [options] <query1> [query2] ...
```

## Options

- `-c, --cache-dir <path>`
  - Cache root to search for `cimport.zig` files.
  - Default: `C:\Projects\Zig\ZigCache\o`

- `-s, --source <path>`
  - Search `.zig` files under this path (repeatable).
  - If used, source mode is enabled.
  - If provided without a value (`-s` alone), Zig stdlib path is auto-detected.

- `--source-std`
  - Force stdlib auto-detection (same intent as `-s` with no value).

- `-m, --min-size <bytes>`
  - Minimum `cimport.zig` size to consider (cache mode).
  - Default: `100000`

- `--max-results <n>`
  - Source mode only.
  - Cap printed matches per query while keeping true total counts.

- `-k, --kind <value>`
  - Line-kind filter (repeatable).
  - Supports comma-lists: `--kind pub-fn,fn`
  - Allowed values:
    - `fn`
    - `pub-fn`
    - `const`
    - `test`
  - If omitted, all line kinds are eligible.

- `-e, --exact`
  - Exact identifier matching (word-boundary semantics).

- `-o, --output <file>`
  - Write report to file instead of stdout.

- `-h, --help`
  - Show built-in help.

## Modes

### 1) Cache Mode (`cimport.zig`)

Used when `--source` is not provided.

Finds the best `cimport.zig` under `--cache-dir` by:
1. Number of query matches (higher is better)
2. File size (tie-breaker)

Example:

```powershell
.\zig-out\bin\zighelper.exe -c C:\Projects\Zig\ZigCache -e ImGui_BeginChild ImGui_EndChild
```

### 2) Source Mode (`.zig` files)

Used when `--source` is provided.

Scans `.zig` files recursively and outputs:

```text
<path>:<line>: <matched line>
```

Example with explicit stdlib path:

```powershell
.\zig-out\bin\zighelper.exe -s "C:\...\zig-x86_64-windows-0.15.2\lib\std" --max-results 20 -e writer writeAll writeInt
```

Example with auto stdlib detection:

```powershell
.\zig-out\bin\zighelper.exe -s --max-results 20 -e writer writeAll writeInt
```

Example filtering to public function declarations:

```powershell
.\zig-out\bin\zighelper.exe -s -k pub-fn -e writeAll writer
```

Example filtering multiple kinds:

```powershell
.\zig-out\bin\zighelper.exe -s -k pub-fn -k fn --max-results 10 -e writer
```

Equivalent comma-list form:

```powershell
.\zig-out\bin\zighelper.exe -s --kind pub-fn,fn --max-results 10 -e writer
```

## Notes

- `cimport.zig` contains translated C declarations. Zig stdlib APIs are usually not there.
- For Zig stdlib APIs, use source mode (`-s` / `--source-std`).
- If stdlib auto-detect fails, pass `--source <path>` explicitly.
