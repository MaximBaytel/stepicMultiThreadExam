#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include "httpheader.h"
#include "common.h"


class HttpHandler
{
public:
    HttpHandler(int socket,const char* absPath);

    //запускает обработку http запросов ответов на сокете переданном в конструкторе...файлы ищутся по пути absPath
    //-1  error
    //0 - all ready
    //+1 - need futher reading
    int processing();

private:

    //вернуть хидер, считав его из сокета
    //-1  error
    //0 - all ready
    //+1 - need futher reading
    int getRequest(int socket,HttpRequestHeader& header);

    //получить тело запроса из сокета..
    //вертнёт false если ошибка на уровне TCP/IP
    //bool getRequestBody(int socket,unsigned int size,std::string& body);

    //сформировать HTTP response
    void responseHeaderToString(HttpResponseHeader& headerResp,std::string& buffer);

    //собственно обработка запроса..
    void processingRequest(const HttpRequestHeader& header,const ByteArray& body,HttpResponseHeader& headerResp,ByteArray& bodyResp);

    //отправить ответ
    bool sendResponse(int socket,HttpResponseHeader& headerResp,ByteArray& bodyResp);

    //read from file
    bool readAll(const std::string& name,ByteArray& data);

    //разобрать стартовую строку запроса
    bool parseRequestLine(const std::string& str,HttpRequestHeader& header);

    //разобрать строку-заголовок запроса
    bool parseHeaderLine(const std::string& str,HttpRequestHeader& header);

    int m_socket;


    static const int m_majorVersion=1;
    static const int m_minorVersion=0;
    //request buffer
    static const size_t m_sizeBuffer = 1024;
    char m_buffer[m_sizeBuffer];
    //current pos in buffer
    int m_pos;
    //path to web server data
    const char* m_path;
    bool m_keepAfterResponse;

};

#endif // WEBSERVER_H
