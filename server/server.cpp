#include "..\helper.cpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // Link with the Winsock library
#include <string.h>
#include <thread>
#include <bits/stdc++.h>
#include <fstream>
#include <mutex>
using namespace std;


mutex mtx[100];

mutex count_mtx;
int clints = 0;
int max_clients = 5;
int sock_fd;


// Define constants for timeout settings
const int DEFAULT_TIMEOUT = 10; // in seconds

struct ClientInfo
{
    int sock;
    chrono::steady_clock::time_point lastActiveTime;
};




void handleGet(int sock, string request)
{
    cout << request.substr(0, 50);
    string fileName = getFileName(request);
    if (!fileExist(fileName))
    {
        mtx[sock].lock();
        commuincate().sendMessage(NOT_FOUND_RES, sock);
        mtx[sock].unlock();
        return;
    }
    else
    {
        if (isImage(fileName))
        {
            mtx[sock].lock();
            commuincate().sendImage(fileName, sock, OK);
            mtx[sock].unlock();
        }
        else
        {
            string fileData = getDataFromFile(fileName);
            string header = addHeader(fileData.size());
            mtx[sock].lock();
            commuincate().sendMessage(OK + header + fileData, sock);
            mtx[sock].unlock();
        }
    }
}
void handlePost(int sock, string request)
{
    string p;
    int i = 0;
    while (request[i] != '\n')
    {
        p += request[i];
        i++;
    }
    p += request[i];
    cout << p;
    string fileName = getFileName(request);
    if (isImage(fileName))
    {
        string image = getDataFromString(request);
        saveImage(image, fileName);
        mtx[sock].lock();
        commuincate().sendMessage(OK, sock);
        mtx[sock].unlock();
    }
    else
    {
        string dataFromRes = getDataFromString(request);
        saveFile(dataFromRes, fileName);
        mtx[sock].lock();
        commuincate().sendMessage(OK, sock);
        mtx[sock].unlock();
    }
    return;
}
void *handle(void *clientInfo)
{

    ClientInfo *info = (ClientInfo *)clientInfo;
    int sock = info->sock;
    
    count_mtx.lock();
    if (clints >= max_clients)
    {
        count_mtx.unlock();
        cout << "Maximum number of clients reached. Closing connection.\n";
        closesocket(sock);
        delete info; // Release memory allocated for ClientInfo
        return nullptr;
    }
    else
    {
        clints++;
        count_mtx.unlock();
    }


    while (1)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        // Set timeout to 1 second (adjust as needed)
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int result = select(sock + 1, &readSet, NULL, NULL, &timeout);
        if (result == SOCKET_ERROR)
        {
            cerr << "Error in select\n";
            break;
        }
        else if (result == 0)
        {
            // No data to receive within the timeout
            // Check for idle timeout
            auto currentTime = chrono::steady_clock::now();
            auto lastActiveTime = info->lastActiveTime;
            auto elapsedTime = chrono::duration_cast<chrono::seconds>(currentTime - lastActiveTime).count();

            if (elapsedTime >= DEFAULT_TIMEOUT)
            {
                cout << "Client idle timeout. Closing connection.\n";
                closesocket(sock);
                count_mtx.lock();
                clints--;
                count_mtx.unlock();
                delete info; // Release memory allocated for ClientInfo
                return nullptr;
            }

            continue;
        }

        // Update the last active time when data is received
        info->lastActiveTime = chrono::steady_clock::now();

        string mes = commuincate().reciveMessage(sock, DEFAULT_TIMEOUT, true);
        if (mes.size() == 0)
        {
            count_mtx.lock();
            clints--;
            count_mtx.unlock();
            delete info; // Release memory allocated for ClientInfo
            return nullptr;
        }

        bool isPost = isPostRequest(mes);
        if (isPost)
        {
            handlePost(sock, mes);
        }
        else
        {   
            handleGet(sock, mes);
        }
    }

    count_mtx.lock();
    clints--;
    count_mtx.unlock();
    delete info; // Release memory allocated for ClientInfo
}



int main(int argc, char const *argv[])
{
    int new_sock, c;
    struct sockaddr_in server;

    int port = stringToInt(argv[1]);
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed\n";
        return -1;
    }
    
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        cerr << "Error creating socket\n";
        WSACleanup(); 
        return -1;
    }

    memset(&server, 0, sizeof(server)); // Zero out structure
    server.sin_family = AF_INET; // IPv4 address family
    server.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    server.sin_port = htons(port); 

    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        cerr << "Error binding socket\n";
        WSACleanup();
        return -1;
    }

    
    if (listen(sock_fd, 10) < 0)
    {
        cerr << "Error listening\n";
        WSACleanup();
        return -1;
    }

cout<<"I am Listening now\n";



while(true){
    struct sockaddr_in client;
    socklen_t clntAddrLen = sizeof(client);
    int clntSock = accept(sock_fd, (struct sockaddr *) &client, &clntAddrLen);
    if (clntSock < 0){
        cerr << "Accept Failed\n";
        WSACleanup();
        return -1;
    }   
    char clntName[INET_ADDRSTRLEN]; // String to contain client address
    if (inet_ntop(AF_INET, &client.sin_addr.s_addr, clntName,sizeof(clntName)) != NULL){
    printf("Handling client %s/%d\n", clntName, ntohs(client.sin_port));
    }else{
    cerr << "Accept Failed\n";}

    ClientInfo *clientInfo = new ClientInfo{clntSock, chrono::steady_clock::now()};
    thread process_conn(&handle, clientInfo);
    process_conn.detach();

}
    return 0;
}
