# HLASM Lexer — Column-Aware Syntax Highlighting for Notepad++

**Column-aware** syntax highlighting for IBM High Level Assembler (HLASM) source in Notepad++. Unlike generic assembly highlighters, it understands the 80-column punch-card layout that HLASM lives by.

> **Two versions live in this repo:**
>
> - **`HLASMLexerL` — the real lexer (recommended).** Registers HLASM as a proper Notepad++ language via the Scintilla/Lexilla `ILexer5` interface. It owns the buffer, so the highlighting just works: it sticks while you type, no manual step, no flicker, and "HLASM" appears in the Language menu like any built-in language.
> - **`HLASMLexer` — the original plugin (deprecated).** Paints styles over the top via `SendMessage` instead of registering as a language, so it fights Notepad++'s own lexing (the "highlighting comes and goes" problem). **It is deprecated and will be removed.** Use `HLASMLexerL`.

## What It Does

HLASM source follows a rigid column layout inherited from 80-column punch cards:

```
Columns 1-71     Content area (label, operation, operand, remarks)
Column  72       Continuation marker (non-blank = line continues)
Columns 73-80    Sequence number
```

Each field is highlighted by its **column position**, not just by keywords:

| Field        | Colour          | Description                            |
|--------------|-----------------|----------------------------------------|
| Label        | Dark blue, bold | Symbols starting in column 1           |
| Operation    | Blue            | Instruction/directive after label      |
| Operand      | Black           | Arguments to the operation             |
| Remarks      | Green, italic   | Comments after the operand field       |
| String       | Red             | Quoted constants (`C'HELLO'`, `X'FF'`) |
| Number       | Orange          | Numeric literals                       |
| Register     | Purple          | R0–R15                                 |
| Continuation | Grey on yellow  | Non-blank character in column 72       |
| Sequence     | Light grey      | Columns 73–80                          |
| Comment      | Green, italic   | Lines starting with `*` or `.*`        |

Vertical **column-boundary rulers** are drawn at the content/continuation boundary, the continuation/sequence boundary, and the end of the card, and (in the new lexer) only on HLASM buffers.

## Supported Features

- Full-line comments (`*` in column 1, `.*` for macro comments)
- Continuation lines (non-blank in column 72, operand resumes at column 16)
- Multi-line string continuation (tracks open quotes across continuation lines)
- Type-prefixed string constants (`C'...'`, `X'...'`, `B'...'`, `F'...'`, etc.)
- Parenthesised sub-expressions in operands (e.g. `AIF ('&REG' EQ '').NOREG`)
- Macro variable references (`&VAR`) and sequence symbols (`.LABEL`)
- Register detection (R0–R15, case-insensitive)

## Installation (recommended: the lexer)

### Requirements

- **Notepad++ 8.4 or newer** (the Scintilla 5 / Lexilla versions), 64-bit.

### Steps

1. **Close Notepad++** completely.
2. Copy the lexer DLL into its own folder (the folder name must match the DLL):
   ```
   C:\Program Files\Notepad++\plugins\HLASMLexerL\HLASMLexerL.dll
   ```
3. Copy the styling config (this maps the styles to colours and associates the file types):
   ```
   C:\Program Files\Notepad++\plugins\Config\HLASMLexerL.xml
   ```
4. **Restart Notepad++.** "HLASM" now appears in the **Language** menu.
5. Open an HLASM source file (`.mlc`, `.mac`, `.cpy`). It highlights automatically and stays highlighted, no further action.

### File extensions

`.mlc`, `.mac` and `.cpy` are associated with HLASM out of the box (via the `ext` attribute in `HLASMLexerL.xml`). To add more, edit that attribute and restart.

Want `.asm` too? Add `asm` to the `ext` list in `HLASMLexerL.xml`, and remove `asm` from the built-in Assembly language under **Settings → Style Configurator → Language: Assembly** so the two don't both claim it. (Unlike the old plugin, the lexer registers properly, so this is just an extension-ownership choice, not a fight.)

## Why a real lexer

The original plugin painted styles directly onto Scintilla. That works until the document also has a language assigned, at which point Notepad++ re-lexes the buffer and repaints over the top, and the highlighting flickers on and off "at whim". The fix used to be a manual dance: set Language → None, then re-apply.

The `ILexer5` version registers HLASM as the buffer's actual lexer, so Notepad++ has nothing painting on top. It just sticks. That is the whole reason the old plugin is being retired.

## Building from Source

### Requirements

- MinGW-w64 (64-bit GCC for Windows).

### Build the lexer

```bat
g++ -c -Wall -Wextra -O2 -std=c++11 -DUNICODE -D_UNICODE -Isrc -o obj\hlasm_ilexer.o src\hlasm_ilexer.cpp
g++ -shared -static -o bin\HLASMLexerL.dll obj\hlasm_ilexer.o exports_lexer.def -s -luser32
```

Or just run `build_lexer.bat`. (`build.bat` still builds the deprecated `HLASMLexer` plugin for now.)

## Architecture

The lexer is a single translation unit that:

1. Implements Scintilla's `ILexer5` interface, reusing the column-parsing logic in `hlasm_core.h` (the same brain the old plugin used).
2. Exports the Lexilla entry points (`CreateLexer`, `GetLexerCount`, `GetLexerName`) so Notepad++ discovers "HLASM" as a language.
3. Carries a small Notepad++ plugin shim alongside, which is how custom lexers are loaded since Notepad++ moved to Scintilla 5 / Lexilla, plus it draws the card-boundary rulers (a view setting, not part of lexing) on HLASM buffers.

The Scintilla/Lexilla headers in `src/scintilla/` are vendored from Notepad++'s own copy, so the interface matches the exact version Notepad++ ships. And yes, despite the old comment in the source about "cross-compiler ABI roulette", `ILexer5` builds fine with MinGW on x64, because 64-bit Windows has one calling convention. The ABI fear was a 32-bit ghost story.

## Limitations

- **No keyword colouring.** `MVC`, `LA`, `STM` are all the same blue. Highlighting is by column position, not by matching mnemonics against a dictionary.
- **No semantic analysis.** The parser follows the column rules; it doesn't know a DC operand from a macro parameter.
- **Fixed-point and bit-level constructs** (e.g. `POS`, scaled fixed-point) aren't specially handled.

## Licence

Apache 2.0 — see [LICENSE](LICENSE). The vendored Scintilla/Lexilla headers in `src/scintilla/` are under their own permissive licence (see the header comments).
