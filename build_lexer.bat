@echo off
REM  Build HLASMLexerL.dll - the real ILexer5 / Lexilla lexer (+ NPP plugin glue)
REM  Needs MinGW-w64 g++ in PATH. 64-bit Notepad++ only.

where g++ >nul 2>&1 || (echo g++ not found - install MinGW-w64 & goto :end)

if not exist obj mkdir obj
if not exist bin mkdir bin

echo Compiling lexer...
g++ -c -Wall -Wextra -O2 -std=c++11 -DUNICODE -D_UNICODE -Isrc -o obj\hlasm_ilexer.o src\hlasm_ilexer.cpp
if errorlevel 1 goto :fail

echo Linking...
g++ -shared -static -o bin\HLASMLexerL.dll obj\hlasm_ilexer.o exports_lexer.def -s -luser32
if errorlevel 1 goto :fail

echo.
echo Done: bin\HLASMLexerL.dll
echo.
echo Install:
echo   1. Copy bin\HLASMLexerL.dll       to  Notepad++\plugins\HLASMLexerL\HLASMLexerL.dll
echo   2. Copy config\HLASMLexerL.xml     to  Notepad++\plugins\Config\HLASMLexerL.xml
echo   3. Restart Notepad++. "HLASM" should appear in the Language menu.
goto :end

:fail
echo BUILD FAILED
:end
