#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") 

// #include <sys/socket.h>
// #include <net/ethernet.h>
// #include <arpa/inet.h>
#include <string.h>
#include "sstream"
#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

const string NOT_FOUND_RES = "HTTP/1.1 404 Not Found\r\n";
const string OK = "HTTP/1.1 200 OK\r\n";

const string DEFALUT_PAGE = "s3.html";
int BUFFER_SIZE = 100000;
int DEFUALT_TIME = 5 * 1000; // 5  S
int MIN_TIME = 1000;


string intToString(int n)
{
    stringstream ss;
    ss << n;
    string s;
    ss >> s;
    return s;
}
string addHeader(int size)
{
    return "Content-Length: " + intToString(size) + "\r\n";
}
bool isNum(char c)
{
    return c >= '0' and c <= '9';
}
int getLen(string str)
{
    int n = str.size();
    int i = 0;
    for (; i < n; i++)
    {
        if (str[i] == ':')
            break;
    }
    i += 2;
    int num = 0;
    while (i < n and isNum(str[i]))
    {
        num *= 10;
        num += str[i] - '0';
        i++;
    }
    return num;
}

class commuincate
{
public:
    bool sendMessage(string str, int to)
    {
        int res = send(to, str.c_str(), str.size(), 0);
        if (res < 0)
        {
            cout << "sending " << str.substr(0, 100) << endl;
            puts("error while sending ");
            return false;
        }
        return true;
    }
    
    string reciveMessage(int from, int timeOut = 0, bool addTimeOut = false)
    {

        char c[BUFFER_SIZE] = {0};
        int res;
        string str;
        int len = -1;
        bool setLen = false;
        while (1)
        {

            if (addTimeOut)
            {
                struct pollfd fds[200];
                int nfds = 1;
                memset(fds, 0, sizeof(fds));
                fds[0].fd = from;
                fds[0].events = POLLIN;
                int rc = WSAPoll(fds, nfds, timeOut);
                if (rc == 0)
                {
                    cout << "time out finish  " << timeOut << endl;
                    close(from);
                    return "";
                }
            }
            res = recv(from, c, BUFFER_SIZE, 0);
            if (res < 0)
            {
                puts("error while receving ");
                return "";
            }
            if (res == 0)
            {
                puts("connection end");
                return "";
            }
            for (int i = 0; i < res; i++)
            {
                str.push_back(c[i]);
            }
            if (!setLen)
            {
                setLen = true;
                len = getLen(str);
            }
            len -= res;
            if (len <= 0)
            {
                break;
            }
        }

        return str;
    }
    bool sendImage(string path, int sock, string message)
    {

        std::ifstream input(path, std::ios::binary);

        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
        int n = buffer.size();
        string header = addHeader(n);

        char c[n + message.size() + header.size()] = {0};
        int cur = 0;
        for (int i = 0; i < message.size(); i++)
        {
            c[cur++] = message[i];
        }
        for (int i = 0; i < header.size(); i++)
        {
            c[cur++] = header[i];
        }
        for (int i = 0; i < buffer.size(); i++)
        {
            c[cur++] = buffer[i];
        }
        int res = send(sock, c, cur, 0);
        if (res < 0)
        {
            puts("error while sending ");
            return false;
        }
        return true;
    }
};

bool IsEmptyLine(string str)
{
    int n = str.size();
    for (int i = 0; i < n; i++)
    {
        if (str[i] == '\n')
            continue;
        if (str[i] == '\r')
            continue;
        if (str[i] == ' ')
            continue;
        return false;
    }
    return true;
}

string getFileName(string data)
{
    int n = data.size();
    string ans;
    int i = 0;
    bool lastT = false;
    while (data[i] == ' ')
        i++;
    for (; i < n; i++)
    {
        if (data[i] == ' ' || data[i] == '\n')
            break;
    }
    i++;
    for (; i < n and data[i] != ' ' and data[i] != '\n'; i++)
    {
        ans += data[i];
    }
    if (ans == "/")
    {
        return DEFALUT_PAGE;
    }
    if (ans[0] == '/')
    {
        ans = ans.substr(1);
    }
    return ans;
}




bool isOk(string res)
{
    for (char c : res)
    {
        if (c == '2')
            return true;
        else if (c == '4')
            return false;
    }
    return false;
}

int stringToInt(string s)
{
    stringstream stream(s);
    int x = 0;
    stream >> x;
    return x;
}
bool isPostRequest(string data)
{

    for (int i = 0; i < data.size(); i++)
    {
        if (data[i] == 'g' || data[i] == 'G')
            return 0; // get
        else if (data[i] == 'p' || data[i] == 'P')
            return 1; // post
    }
    assert(false);
}

bool fileExist(const std::string &name)
{

    if (FILE *file = fopen(name.c_str(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}

string getDataFromFile(string fileName)
{
    std::ifstream infile(fileName);
    string line;
    string data;
    try
    {
        while (infile.is_open() and getline(infile, line))
        {
            if (data.size())
            {
                data += "\n";
            }
            data += line;
        }
    }
    catch (int e)
    {
    }
    infile.close();

    return data;
}

bool isImage(string name)
{
    int n = name.size();
    for (int i = 0; i < n; i++)
    {
        if (name[i] == '.')
        {
            if (name[i + 1] == 'h')
                return 0;
            if (name[i + 1] == 't')
                return 0;
            return 1;
        }
    }
    return 0;
}

vector<unsigned char> getDataFromVector(vector<unsigned char> vec)
{
    vector<unsigned char> res;
    int s = 0;
    int c = 0;
    while (c <= 1)
    {
        if (vec[s] == '\n')
        {
            c++;
        }
        s++;
    }
    for (int i = s; i < vec.size(); i++)
    {
        res.push_back(vec[i]);
    }
    return res;
}


string getDataFromString(string str)
{
    string res;
    int s = 0;
    int c = 0;
    while (c <= 1)
    {
        if (str[s] == '\n')
        {
            c++;
        }
        s++;
    }
    for (int i = s; i < str.size(); i++)
    {
        res.push_back(str[i]);
    }
    return res;
}

void saveImage(string vec, string fileName)
{

    std::ofstream file(fileName, std::ios::binary);
    char c[vec.size()] = {0};
    for (int i = 0; i < vec.size(); i++)
    {
        c[i] = vec[i];
    }
    file.write(c, vec.size());
}
string trim_left(string str)
{
    string ans;
    for (int i = 0; i < str.size(); i++)
    {
        if (ans.size() || str[i] != ' ')
        {
            ans += str[i];
        }
    }
    return ans;
}
string changeFormat(string str)
{
    if (str[0] == 'G' || str[0] == 'P')
        return str;
    if (str[7] == 'g')
    {
        str = "GET " + str.substr(string("client_get ").size());
    }
    else
    {
        str = "POST " + str.substr(string("client_post ").size());
    }
    return str;
}


string trim(string str)
{
    str = trim_left(str);
    reverse(str.begin(), str.end());
    str = trim_left(str);
    reverse(str.begin(), str.end());
    return changeFormat(str) + "\n";
}

void saveFile(string data, string name)
{
    ofstream outfile(name);

    outfile << data;

    outfile.close();
}
