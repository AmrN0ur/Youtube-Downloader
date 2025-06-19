#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <regex>
#include <fstream>
#include <limits>
#include <iomanip>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #include <conio.h>
    #define MKDIR(dir) _mkdir(dir)
    #define STAT _stat
    #define STAT_STRUCT struct _stat
    #define SLEEP(ms) Sleep(ms)
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #include <termios.h>
    #define MKDIR(dir) mkdir(dir, 0755)
    #define STAT stat
    #define STAT_STRUCT struct stat
    #define SLEEP(ms) usleep(ms * 1000)
#endif

using namespace std;

// ===============================================
// Utility Functions
// ===============================================
void waitForKeyPress() {
    cout << "\nPress any key to continue...";
    #ifdef _WIN32
        _getch();
    #else
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    #endif
    cout << endl;
}

// ===============================================
// Custom Exception Classes
// ===============================================
class DownloadException : public runtime_error {
public:
    DownloadException(const string& message) : runtime_error(message) {}
};

class FileSystemException : public runtime_error {
public:
    FileSystemException(const string& message) : runtime_error(message) {}
};

// ===============================================
// Secure Command Builder Class
// ===============================================
class SecureCommandBuilder {
private:
    // Clean text from dangerous characters
    string sanitizeInput(const string& input) {
        string result = input;
        // Remove dangerous characters
        vector<char> dangerousChars = {'|', '&', ';', '(', ')', '$', '`', '"', '\'', '<', '>', '\n', '\r'};
        
        for (char c : dangerousChars) {
            result.erase(remove(result.begin(), result.end(), c), result.end());
        }
        
        return result;
    }
    
    // Add quotes safely
    string escapeForShell(const string& input) {
        string cleaned = sanitizeInput(input);
        return "\"" + cleaned + "\"";
    }

public:
    string buildDownloadCommand(const string& downloaderPath, 
                              const string& url, 
                              const string& outputDir,
                              int quality = 0,
                              const string& format = "mp4",
                              bool audioOnly = false) {
        
        // Clean all inputs
        string cleanUrl = sanitizeInput(url);
        string cleanPath = escapeForShell(downloaderPath);
        string cleanOutput = escapeForShell(outputDir + "/%(uploader)s - %(title)s.%(ext)s");
        
        string command = cleanPath + " -o " + cleanOutput;
        
        if (audioOnly) {
            command += " -f \"bestaudio/best\" --extract-audio --audio-format mp3";
        } else {
            if (quality == 0) {
                command += " -f \"best[height<=2160][ext=" + format + "]/best[height<=2160]/best\"";
            } else {
                command += " -f \"best[height<=" + to_string(quality) + "][ext=" + format + "]/best[height<=" + to_string(quality) + "]/best\"";
            }
        }
        
        command += " --write-info-json --write-thumbnail --embed-subs --write-auto-sub";
        command += " " + escapeForShell(cleanUrl);
        
        return command;
    }
};

// ===============================================
// Logger Class
// ===============================================
class DownloadLogger {
private:
    string logFile;
    
public:
    DownloadLogger(const string& logPath) : logFile(logPath) {}
    
    void logDownload(const string& url, int quality, const string& format, bool success) {
        ofstream log(logFile, ios::app);
        if (log.is_open()) {
            auto now = chrono::system_clock::now();
            time_t now_time = chrono::system_clock::to_time_t(now);
            
            log << "[" << put_time(localtime(&now_time), "%Y-%m-%d %H:%M:%S") << "] "
                << (success ? "Download Successful" : "Download Failed") << " - "
                << "URL: " << url << " | "
                << "Quality: " << (quality == 0 ? "Best" : to_string(quality) + "p") << " | "
                << "Format: " << format << endl;
            log.close();
        }
    }
    
    void showHistory(int maxEntries = 20) {
        cout << "📜 Download History" << endl;
        cout << "──────────────────────────────────────────" << endl;
        
        ifstream log(logFile);
        if (!log.is_open()) {
            cout << "No download history available." << endl;
            return;
        }
        
        string line;
        int count = 0;
        while (getline(log, line) && count < maxEntries) {
            cout << line << endl;
            count++;
        }
        
        if (count == 0) {
            cout << "No download history." << endl;
        }
        
        log.close();
    }
};

// ===============================================
// File System Manager Class
// ===============================================
class FileSystemManager {
public:
    static bool fileExists(const string& path) {
        STAT_STRUCT buffer;
        return (STAT(path.c_str(), &buffer) == 0);
    }

    static bool isExecutable(const string& path) {
        STAT_STRUCT buffer;
        if (STAT(path.c_str(), &buffer) != 0) return false;
        
        #ifdef _WIN32
            return true; // Windows doesn't use Unix permissions
        #else
            return (buffer.st_mode & S_IXUSR) != 0;
        #endif
    }

    static void createDirectory(const string& path) {
        STAT_STRUCT st;
        if (STAT(path.c_str(), &st) != 0) {
            if (MKDIR(path.c_str()) != 0) {
                throw FileSystemException("Failed to create directory: " + path);
            } else {
                cout << "📁 Directory created: " << path << endl;
            }
        }
    }

    static void makeExecutable(const string& path) {
        #ifndef _WIN32
            string chmodCmd = "chmod +x \"" + path + "\"";
            system(chmodCmd.c_str());
        #endif
    }

    static void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    static string detectDownloaderPath(bool verboseMode = false) {
        vector<string> possiblePaths = {
            #ifdef _WIN32
                "libs\\yt-dlp.exe",
                "yt-dlp.exe",
                "bin\\yt-dlp.exe"
            #else
                "./libs/yt-dlp",
                "./yt-dlp",
                "/usr/local/bin/yt-dlp",
                "/usr/bin/yt-dlp",
                "./bin/yt-dlp"
            #endif
        };

        for (const string& path : possiblePaths) {
            if (fileExists(path)) {
                if (!isExecutable(path)) {
                    makeExecutable(path);
                }
                if (verboseMode) {
                    cout << "✅ Found yt-dlp at: " << path << endl;
                }
                return path;
            }
        }

        // Instead of exit, throw exception
        string errorMsg = "❌ Error: yt-dlp not found anywhere!\n";
        errorMsg += "Please install yt-dlp or place it in one of these locations:\n";
        for (const string& path : possiblePaths) {
            errorMsg += "  - " + path + "\n";
        }
        
        throw FileSystemException(errorMsg);
    }
};

// ===============================================
// Input Validator Class
// ===============================================
class InputValidator {
private:
    vector<int> validQualities = {144, 240, 360, 480, 720, 1080, 1440, 2160};
    vector<string> validFormats = {"mp4", "webm", "mkv", "avi"};

public:
    bool isValidQuality(int quality) {
        return quality == 0 || find(validQualities.begin(), validQualities.end(), quality) != validQualities.end();
    }

    bool isValidFormat(const string& format) {
        return find(validFormats.begin(), validFormats.end(), format) != validFormats.end();
    }

    // URL validation with improved patterns
    bool isValidURL(const string& url) {
        vector<regex> patterns = {
            regex(R"(^(https?://)?(www\.)?youtube\.com/watch\?v=[a-zA-Z0-9_-]{11})"),
            regex(R"(^(https?://)?(www\.)?youtu\.be/[a-zA-Z0-9_-]{11})"),
            regex(R"(^(https?://)?(www\.)?youtube\.com/playlist\?list=[a-zA-Z0-9_-]+)"),
            regex(R"(^(https?://)?(www\.)?youtube\.com/channel/[a-zA-Z0-9_-]+)"),
            regex(R"(^(https?://)?(www\.)?youtube\.com/@[a-zA-Z0-9_.-]+)")
        };
        
        for (const auto& pattern : patterns) {
            if (regex_match(url, pattern)) {
                return true;
            }
        }
        return false;
    }

    string trimString(const string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    string toLowerCase(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    vector<int> getValidQualities() const { return validQualities; }
    vector<string> getValidFormats() const { return validFormats; }
};

// ===============================================
// User Interface Class
// ===============================================
class UserInterface {
private:
    InputValidator validator;
    
public:
    void displayHeader() {
        cout << "===============================================" << endl;
        cout << "|==         YouTube Downloader Pro           ==|" << endl;
        cout << "|==            Enhanced Version              ==|" << endl;
        cout << "===============================================" << endl << endl;
        cout << "📹 Professional YouTube Video Downloader" << endl;
        cout << "🎯 Supports videos, playlists, and channels" << endl;
        cout << "💾 Download folder: downloads" << endl << endl;
    }
    
    void displayMainMenu() {
        cout << "┌─────────────────────────────────────────┐" << endl;
        cout << "│                Main Menu                │" << endl;
        cout << "├─────────────────────────────────────────┤" << endl;
        cout << "│ 1. Download single video                │" << endl;
        cout << "│ 2. Download playlist                    │" << endl;
        cout << "│ 3. Download channel videos              │" << endl;
        cout << "│ 4. Download audio only                  │" << endl;
        cout << "│ 5. Settings                             │" << endl;
        cout << "│ 6. Show download history                │" << endl;
        cout << "│ 0. Exit                                 │" << endl;
        cout << "└─────────────────────────────────────────┘" << endl;
        cout << "Choose option: ";
    }
    
    void displayQualities() {
        auto qualities = validator.getValidQualities();
        cout << "┌─────────────────────────────────────────┐" << endl;
        cout << "│            Available Qualities          │" << endl;
        cout << "├─────────────────────────────────────────┤" << endl;
        for (int quality : qualities) {
            cout << "│ " << setw(4) << quality << "p - ";
            switch (quality) {
                case 144: cout << "Very Low Quality (Mobile)      │" << endl; break;
                case 240: cout << "Low Quality                    │" << endl; break;
                case 360: cout << "Medium Quality                 │" << endl; break;
                case 480: cout << "Standard Quality (SD)          │" << endl; break;
                case 720: cout << "High Quality (HD)              │" << endl; break;
                case 1080: cout << "Very High Quality (Full HD)   │" << endl; break;
                case 1440: cout << "Ultra Quality (2K)            │" << endl; break;
                case 2160: cout << "Crystal Quality (4K)          │" << endl; break;
            }
        }
        cout << "│  0   - Best available quality           │" << endl;
        cout << "└─────────────────────────────────────────┘" << endl << endl;
    }
    
    void displayFormats() {
        auto formats = validator.getValidFormats();
        cout << "Available formats:" << endl;
        for (size_t i = 0; i < formats.size(); ++i) {
            cout << "  " << i + 1 << ". " << formats[i] << endl;
        }
        cout << endl;
    }
    
    string getVideoURL() {
        string url;
        cout << "📎 Enter video/playlist/channel URL:" << endl;
        cout << ">> ";
        getline(cin, url);
        return validator.trimString(url);
    }
    
    int getVideoQuality() {
        int quality;
        displayQualities();
        cout << "🎬 Enter video quality (or 0 for best): ";
        while (!(cin >> quality) || !validator.isValidQuality(quality)) {
            cout << "❌ Invalid input! Please enter a valid quality: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        return quality;
    }
    
    string getVideoFormat() {
        int choice;
        auto formats = validator.getValidFormats();
        displayFormats();
        cout << "📼 Choose format (1-" << formats.size() << "): ";
        while (!(cin >> choice) || choice < 1 || choice > static_cast<int>(formats.size())) {
            cout << "❌ Invalid choice! Please choose 1-" << formats.size() << ": ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        return formats[choice - 1];
    }
    
    void showProgress() {
        cout << "🔄 Downloading";
        for (int i = 0; i < 3; ++i) {
            cout << "." << flush;
            SLEEP(500);
        }
        cout << endl;
    }
};

// ===============================================
// Main Downloader Class
// ===============================================
class VideoDownloader {
private:
    string downloaderPath;
    DownloadLogger* logger;
    InputValidator validator;
    SecureCommandBuilder commandBuilder;
    
public:
    VideoDownloader(const string& path, DownloadLogger* log) 
        : downloaderPath(path), logger(log) {}
    
    bool download(const string& url, int quality, const string& format, 
                 const string& outputDir, bool audioOnly = false, bool verboseMode = false) {
        
        try {
            if (!validator.isValidURL(url)) {
                throw DownloadException("Invalid URL format!\nSupported formats:\n"
                                      "  - Single video: youtube.com/watch?v=...\n"
                                      "  - Short link: youtu.be/...\n"
                                      "  - Playlist: youtube.com/playlist?list=...\n"
                                      "  - Channel: youtube.com/channel/...\n"
                                      "  - Username: youtube.com/@username");
            }

            string command = commandBuilder.buildDownloadCommand(
                downloaderPath, url, outputDir, quality, format, audioOnly
            );

            if (verboseMode) {
                cout << "🔧 Command: " << command << endl;
            }

            cout << "🚀 Starting download..." << endl;

            int result = system(command.c_str());
            bool success = (result == 0);

            if (success) {
                cout << "✅ Download successful!" << endl;
                cout << "📁 Files saved to: " << outputDir << endl;
            } else {
                // More detailed failure reasons
                string errorMsg = "❌ Download failed! Possible reasons:\n";
                errorMsg += "  1. Internet connection problem\n";
                errorMsg += "  2. Invalid or unavailable URL\n";
                errorMsg += "  3. Video not available in your region\n";
                errorMsg += "  4. Requested quality not available\n";
                errorMsg += "  5. Disk space full\n";
                
                throw DownloadException(errorMsg);
            }

            // Log the result
            if (logger) {
                logger->logDownload(url, quality, format, success);
            }

            return success;
            
        } catch (const DownloadException& e) {
            cerr << e.what() << endl;
            waitForKeyPress();
            if (logger) {
                logger->logDownload(url, quality, format, false);
            }
            return false;
        } catch (const exception& e) {
            cerr << "❌ Unexpected error: " << e.what() << endl;
            waitForKeyPress();
            if (logger) {
                logger->logDownload(url, quality, format, false);
            }
            return false;
        }
    }
};

// ===============================================
// Main Application Class
// ===============================================
class YouTubeDownloaderPro {
private:
    string downloaderPath;
    string downloadDir = "downloads";
    string logFile = "logs/download_log.txt";
    bool verboseMode = false;
    
    DownloadLogger* logger;
    VideoDownloader* downloader;
    UserInterface ui;
    
public:
    YouTubeDownloaderPro() {
        try {
            // Create required directories
            FileSystemManager::createDirectory(downloadDir);
            FileSystemManager::createDirectory("logs");
            
            // Detect yt-dlp path
            downloaderPath = FileSystemManager::detectDownloaderPath(verboseMode);
            
            // Create logger and downloader
            logger = new DownloadLogger(logFile);
            downloader = new VideoDownloader(downloaderPath, logger);
            
        } catch (const exception& e) {
            cerr << e.what() << endl;
            waitForKeyPress();
            exit(1);
        }
    }
    
    ~YouTubeDownloaderPro() {
        delete logger;
        delete downloader;
    }
    
    void showSettings() {
        cout << "⚙️  Settings" << endl;
        cout << "──────────────────────────────────────────" << endl;
        cout << "Current download directory: " << downloadDir << endl;
        cout << "Verbose mode: " << (verboseMode ? "Enabled" : "Disabled") << endl;
        cout << "Downloader path: " << downloaderPath << endl;
        cout << endl;
        
        cout << "1. Change download directory" << endl;
        cout << "2. Toggle verbose mode" << endl;
        cout << "3. Test downloader" << endl;
        cout << "0. Back to main menu" << endl;
        cout << "Choose option: ";
        
        int choice;
        cin >> choice;
        cin.ignore();
        
        switch (choice) {
            case 1: {
                cout << "Enter new download directory: ";
                string newDir;
                getline(cin, newDir);
                if (!newDir.empty()) {
                    downloadDir = newDir;
                    try {
                        FileSystemManager::createDirectory(downloadDir);
                        cout << "✅ Download directory changed to: " << downloadDir << endl;
                    } catch (const exception& e) {
                        cerr << "❌ Failed to create directory: " << e.what() << endl;
                        waitForKeyPress();
                    }
                }
                break;
            }
            case 2:
                verboseMode = !verboseMode;
                cout << "✅ Verbose mode " << (verboseMode ? "enabled" : "disabled") << endl;
                break;
            case 3: {
                cout << "🧪 Testing downloader..." << endl;
                string testCmd = "\"" + downloaderPath + "\" --version";
                system(testCmd.c_str());
                break;
            }
        }
    }
    
    void run() {
        ui.displayHeader();
        
        while (true) {
            ui.displayMainMenu();
            
            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "❌ Invalid input! Please enter a number." << endl;
                waitForKeyPress();
                continue;
            }
            cin.ignore();
            
            switch (choice) {
                case 1: {
                    cout << "\n🎬 Download Single Video" << endl;
                    cout << "──────────────────────────────────────────" << endl;
                    
                    string url = ui.getVideoURL();
                    if (url.empty()) {
                        cout << "❌ No URL entered!" << endl;
                        waitForKeyPress();
                        break;
                    }
                    
                    int quality = ui.getVideoQuality();
                    string format = ui.getVideoFormat();
                    
                    cout << "\n📋 Download Summary" << endl;
                    cout << "URL: " << url << endl;
                    cout << "Quality: " << (quality == 0 ? "Best Available" : to_string(quality) + "p") << endl;
                    cout << "Format: " << format << endl;
                    cout << "Destination: " << downloadDir << endl;
                    cout << "──────────────────────────────────────────" << endl;
                    
                    downloader->download(url, quality, format, downloadDir, false, verboseMode);
                    break;
                }
                case 2: {
                    cout << "\n📑 Download Playlist" << endl;
                    cout << "──────────────────────────────────────────" << endl;
                    
                    string url = ui.getVideoURL();
                    if (url.empty() || url.find("playlist") == string::npos) {
                        cout << "❌ Please enter a valid playlist URL!" << endl;
                        waitForKeyPress();
                        break;
                    }
                    
                    int quality = ui.getVideoQuality();
                    string format = ui.getVideoFormat();
                    
                    downloader->download(url, quality, format, downloadDir, false, verboseMode);
                    break;
                }
                case 3: {
                    cout << "\n📺 Download Channel Videos" << endl;
                    cout << "──────────────────────────────────────────" << endl;
                    
                    string url = ui.getVideoURL();
                    if (url.empty()) {
                        cout << "❌ No URL entered!" << endl;
                        waitForKeyPress();
                        break;
                    }
                    
                    int quality = ui.getVideoQuality();
                    string format = ui.getVideoFormat();
                    
                    downloader->download(url, quality, format, downloadDir, false, verboseMode);
                    break;
                }
                case 4: {
                    cout << "\n🎵 Download Audio Only" << endl;
                    cout << "──────────────────────────────────────────" << endl;
                    
                    string url = ui.getVideoURL();
                    if (url.empty()) {
                        cout << "❌ No URL entered!" << endl;
                        waitForKeyPress();
                        break;
                    }
                    
                    downloader->download(url, 0, "mp3", downloadDir, true, verboseMode);
                    break;
                }
                case 5:
                    FileSystemManager::clearScreen();
                    showSettings();
                    break;
                case 6:
                    FileSystemManager::clearScreen();
                    logger->showHistory();
                    break;
                case 0:
                    cout << "👋 Thank you for using YouTube Downloader Pro!" << endl;
                    return;
                default:
                    cout << "❌ Invalid choice! Please choose 0-6." << endl;
                    waitForKeyPress();
            }
            
            cout << "\nPress Enter to continue...";
            cin.get();
            FileSystemManager::clearScreen();
        }
    }
};

// ===============================================
// Main Function
// ===============================================
int main() {
    try {
        // Setup UTF-8 encoding for console
        #ifdef _WIN32
            system("chcp 65001 > nul");
            SetConsoleOutputCP(CP_UTF8);
        #endif
        
        // Create and run the application
        YouTubeDownloaderPro app;
        FileSystemManager::clearScreen();
        app.run();
        
    } catch (const exception& e) {
        cerr << "❌ Fatal error: " << e.what() << endl;
        waitForKeyPress();
        return 1;
    } catch (...) {
        cerr << "❌ Unknown error occurred!" << endl;
        waitForKeyPress();
        return 1;
    }
    
    return 0;
}