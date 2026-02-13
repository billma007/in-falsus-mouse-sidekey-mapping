# In Falsus Mouse Side-Key Mapping Tool

A lightweight C++ utility designed specifically for the rhythm game **"In Falsus"**. It maps your mouse side buttons (XButton1/Forward and XButton2/Backward) to `Left-Shift` and `Space` keys.

## Why this tool?
Unlike generic key mapping software, this tool uses **Hardware Scan Codes** via low-level Windows hooks. This ensures that "holding" a key (long press) is correctly recognized by the game engine, which is crucial for hitting long notes in rhythm games. Generic virtual key mapping often fails to sustain the "pressed" state in certain DirectX/Unity games.

## Features
- **Zero Latency**: Direct low-level hook implementation.
- **Hold Logic**: Supports long-pressing for "hold" notes without interruption.
- **Toggle Hotkey**: Quickly enable/disable the mapper using `Ctrl+Alt+H` (customizable).
- **Swap Mapping**: Switch which button triggers Space vs. Shift.
- **Bilingual UI**: Supports English and Chinese (Simplified).

## Requirements
- Windows 10 / 11
- Administrator privileges (required to send input to games running as Admin).

## Build Instructions
This project is a single-file C++ application using the Win32 API. You can compile it using MinGW (g++) or MSVC.

### Using MinGW (g++)
```bash
g++ main.cpp -o "InFalsusMapper.exe" -mwindows -municode -lcomctl32 -static
```

## Contact
- Email: ma237103015@126.com
- QQ: 36937975
