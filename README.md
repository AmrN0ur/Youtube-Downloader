# 📥 YouTube Downloader (CUI)

A command-line YouTube video downloader written in C++.  
Supports Windows, Linux, and Android (via Termux).

---

## 🧠 Project Overview

This is a simple console-based (CUI) YouTube video downloader.  
It takes a video URL and uses external tools like `yt-dlp` to download the video.

The project is written entirely in modern C++ and uses standard libraries for input/output, string manipulation, regex matching, and file handling.

---

## 🔧 Features

- ✅ Cross-platform: Windows, Linux, Android (via [Termux](https://termux.dev/))
- ✅ UTF-8 output supported in console
- ✅ Uses `regex` to process and validate URLs
- ✅ Simple user interface
- ✅ Saves settings in `ini` files
- ✅ `yt-dlp` included in the project (no need to install it separately)

---

## 🚀 How to Run

### 🧱 Compile (Linux / Termux)
```bash
g++ main.cpp -o youtube-downloader
./youtube-downloader
```
OR
```
Download The 'binary' Archive in The Releas, its ready to run on all Platforms [ Windows, MacOS, Linux, Android('Termux' or its Alternative) ]
```
