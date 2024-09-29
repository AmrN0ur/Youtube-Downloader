// Youtube Downloader Tool
#include <iostream>
#include <string>
using namespace std;

string url, command;
int q = 0;

bool getVideo(string url = "https://www.youtube.com/watch?v=BfwSLOHlPrw", int q = -1) {
    if (q < 0 && q != 144 || q != 240 || q != 360 || q != 480 || q != 720 || q != 1080) {
        cerr << "\a                                  *= Error: Invalid Quality Input, Chosse one of This List:\n                                  *==  [ 144, 240, 360, 480, 720, 1080 ]" << endl;
        command = "./url_down - f best " + url;
    }
    else command = "./url_down - f " + to_string(q) + " " + url;
}

int main(){
    cout << "                                  =========================================\n";
    cout << "                                  |==       Youtube Downloader Tool     ==|\n";
    cout << "                                  =========================================\n" << endl;

    cout << "                                  == Enter The Vedio URL:\n                                  === >> "; getline(cin, url);
    cout << "                                  == Enter The Vedio Quality:\n                                  === >> "; cin >> q;




    system(command.c_str());

    return 0;
}