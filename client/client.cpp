#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // Link with the Winsock library

#include <string.h>
#include "sstream"
#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include "..\helper.cpp"
#include <bits/stdc++.h>


using namespace std;

mutex mtx;

struct Request
{
    int sock;
    string request;
};


void handlePostRequest(string request, int sock_fd);
void handleGetRequest(string data, int sock_fd);

void handleRequest(void *r)
{
    Request *d = (Request *)r;
    int sock_fd = d->sock;
    string str = d->request;
    bool isPost = isPostRequest(str);
    if (isPost)
    {
        handlePostRequest(trim(str), sock_fd);
    }
    else
    {
        handleGetRequest(trim(str), sock_fd);
    }
    return;
}
void handleGetRequest(string data, int sock_fd)
{
    commuincate().sendMessage(data, sock_fd);
    string fileName = getFileName(data);

    if (isImage(fileName))
    {
        string res = commuincate().reciveMessage(sock_fd);

        if (!isOk(res))
        {
            cout << "get request , NOT FOUND : " << data.substr(0, 100) << endl;

            return;
        }
        string image = getDataFromString(res);
        saveImage(image, fileName);
    }
    else
    {
        string res = commuincate().reciveMessage(sock_fd);
        if (!isOk(res))
        {
            cout << "get request , NOT FOUND : " << data.substr(0, 100) << endl;
            return;
        }
        string dataFromRes = getDataFromString(res);
        saveFile(dataFromRes, fileName);
    }
    cout << "get request is success : " << data.substr(0, 100);

    return;
}
void handlePostRequest(string request, int sock_fd)
{

    string fileName = getFileName(request);
    if (!fileExist(fileName))
    {
        cout << "post request ,file not here  : " << request.substr(0, 100) << endl;
        return;
    }
    else
    {
        if (isImage(fileName))
        {

            commuincate().sendImage(fileName, sock_fd, request);
        }
        else
        {
            string fileData = getDataFromFile(fileName);
            string header = addHeader(fileData.size());
            commuincate().sendMessage(request + header + fileData, sock_fd);
        }
        string res = commuincate().reciveMessage(sock_fd);

        if (isOk(res))
        {
            cout << "post request is success : " << request.substr(0, 100);
        }
        else
        {
            puts("error while sending ");
        }
    }
    return;
}



int defaultPort = 80;

int main(int argc, char const *argv[]){
    const char *serverIp;
    int port;
    serverIp = argv[1];
    if (argc <= 1)
    {
        port = defaultPort;
    }
    else
    {
        port = stringToInt(argv[2]);
    }
    printf("server is %s and the port number is %d \n", serverIp, port);
    fflush(stdout);    
    

    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0)
    {
        cerr << "Failed to initialize Winsock." << endl;
        return -1;
    }

    
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket < 0)
    {
        cerr << "Error creating socket." << endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    int rtnVal = inet_pton(AF_INET, serverIp, &serverAddr.sin_addr.s_addr);
    if (rtnVal <= 0){
        cerr << "inet_pton() failed\n";
        closesocket(clientSocket);
        return -1;
    }
    serverAddr.sin_port = htons(port);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
    {
        cerr << "Error binding socket\n";
        closesocket(clientSocket);
        return -1;
    }


    ifstream inputFile("input.txt");
    if (!inputFile.is_open()) {
        cerr << "Error opening input.txt" << endl;
        return -1;
    }

    string line;
    while (getline(inputFile, line)) {
        cout<<line<<endl;
        if (IsEmptyLine(line)) {
            continue;
        } else {
            Request *r = new Request();
            r->sock = clientSocket;
            r->request = line;
            handleRequest(r);
        }
    }
    // this_thread::sleep_for(chrono::seconds(15));
    shutdown(clientSocket, SD_BOTH);  // Disable both sends and receives
    closesocket(clientSocket);
    inputFile.close();
    return 0;
}
