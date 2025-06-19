#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <regex>
#include <sys/stat.h>
#include <limits>

using namespace std;

class YouTubeDownloader {
private:
    vector<int> validQualities = {144, 240, 360, 480, 720, 1080};
    string downloaderPath;
    string downloadDir = "videos";  // مجلد التحميل

    bool isValidURL(const string& url) {
        regex youtubeRegex(R"(^(https?://)?(www\.)?(youtube\.com/watch\?v=|youtu\.be/)[a-zA-Z0-9_-]{11})");
        return regex_match(url, youtubeRegex);
    }

    bool isValidQuality(int quality) {
        return find(validQualities.begin(), validQualities.end(), quality) != validQualities.end();
    }

    string trimString(const string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    bool fileExistsAndExecutable(const string& path) {
        struct stat sb;
        return (stat(path.c_str(), &sb) == 0 && sb.st_mode & S_IXUSR);
    }

    void ensureDownloadDir() {
        struct stat st;
        if (stat(downloadDir.c_str(), &st) != 0) {
            // إنشاء المجلد
            #ifdef _WIN32
                system((string("mkdir ") + downloadDir).c_str());
            #else
                system((string("mkdir -p ") + downloadDir).c_str());
            #endif
        }
    }

    void detectDownloaderPath() {
    #ifdef _WIN32
        downloaderPath = "libs\\yt-dlp.exe";
    #else
        downloaderPath = "./libs/yt-dlp";
    #endif
        struct stat sb;
        if (stat(downloaderPath.c_str(), &sb) != 0) {
            cerr << "خطأ: لم يتم العثور على yt-dlp في المسار '" << downloaderPath << "'." << endl;
            exit(1);
        }
        if (!(sb.st_mode & S_IXUSR)) {
            cerr << "ملف yt-dlp موجود لكن صلاحيات التنفيذ مفقودة. أحاول إضافة صلاحية التنفيذ..." << endl;
            string chmodCmd = string("chmod +x ") + downloaderPath;
            system(chmodCmd.c_str());
            if (!fileExistsAndExecutable(downloaderPath)) {
                cerr << "فشل في إضافة صلاحية التنفيذ. تأكد يدوياً ثم أعد تشغيل البرنامج." << endl;
                exit(1);
            }
        }
    }

public:
    YouTubeDownloader() {
        detectDownloaderPath();
        ensureDownloadDir();
    }

    void displayHeader() {
        cout << "===========================================" << endl;
        cout << "|==       YouTube Downloader Tool       ==|" << endl;
        cout << "===========================================" << endl << endl;
    }

    void displayQualities() {
        cout << "== Available Qualities:" << endl;
        cout << "=== 144p  - Very Low Quality" << endl;
        cout << "=== 240p  - Low Quality" << endl;
        cout << "=== 360p  - Medium Quality" << endl;
        cout << "=== 480p  - Good Quality" << endl;
        cout << "=== 720p  - High Quality (HD)" << endl;
        cout << "=== 1080p - Very High Quality (Full HD)" << endl;
        cout << "=== 0     - Best Available Quality" << endl << endl;
    }

    string getVideoURL() {
        string url;
        cout << "== Enter Video URL:" << endl;
        cout << "=== >> ";
        getline(cin, url);
        return trimString(url);
    }

    int getVideoQuality() {
        int quality;
        cout << "== Enter Video Quality:" << endl;
        cout << "=== >> ";
        cin >> quality;
        return quality;
    }

    bool downloadVideo(const string& url, int quality) {
        if (!isValidURL(url)) {
            cerr << "\n❌ Error: Invalid URL!" << endl;
            cerr << "Make sure the URL is from YouTube and in correct format" << endl;
            return false;
        }

        // بناء مسار الإخراج
        string outputTemplate = downloadDir + "/%(title)s.%(ext)s";

        string command;
        if (quality == 0) {
            command = downloaderPath + " -o \"" + outputTemplate + "\" -f \"best[height<=1080]\" \"" + url + "\"";
        } else if (isValidQuality(quality)) {
            command = downloaderPath + " -o \"" + outputTemplate + "\" -f \"best[height<=" + to_string(quality) + "]\" \"" + url + "\"";
        } else {
            cerr << "\n❌ Error: Invalid quality!" << endl;
            cerr << "Choose one of the available qualities: ";
            for (int q : validQualities) cout << q << " ";
            cout << "or 0 for best quality" << endl;
            return false;
        }

        cout << "\n🔄 Downloading video..." << endl;
        cout << "⏳ Please wait..." << endl;

        int result = system(command.c_str());

        if (result == 0) {
            cout << "\n✅ Video downloaded successfully to '" << downloadDir << "' folder!" << endl;
            return true;
        } else {
            cerr << "\n❌ Failed to download video!" << endl;
            cerr << "Please check:" << endl;
            cerr << "1. Your internet connection" << endl;
            cerr << "2. That yt-dlp in libs folder is executable" << endl;
            cerr << "3. That the URL is correct and accessible" << endl;
            return false;
        }
    }

    void run() {
        displayHeader();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        string url = getVideoURL();
        if (url.empty()) {
            cerr << "❌ No URL entered!" << endl;
            return;
        }

        displayQualities();
        int quality = getVideoQuality();

        cout << "\n===========================================" << endl;
        cout << "URL: " << url << endl;
        cout << "Quality: " << (quality == 0 ? "Best Available" : to_string(quality) + "p") << endl;
        cout << "Download folder: " << downloadDir << endl;
        cout << "===========================================" << endl;

        if (downloadVideo(url, quality)) cout << "\n🎉 Operation completed successfully!" << endl;

        cout << "\nPress any key to exit...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
    }
};

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    try {
        YouTubeDownloader downloader;
        downloader.run();
    } catch (const exception& e) {
        cerr << "Unexpected error: " << e.what() << endl;
        return 1;
    }
    return 0;
}