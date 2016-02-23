#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include "common.h"
#include "httphandler.h"


using namespace std;

HttpHandler::HttpHandler(int socket, const char *absPath):m_socket(socket),m_pos(0),m_path(absPath),m_keepAfterResponse(false)
{
    memset(m_buffer,0,m_sizeBuffer);
}


//получить и разобрать заголовки..
//-1  error
//0 - all ready
//+1 - need futher reading
int HttpHandler::getRequest(int socket,HttpRequestHeader& header)
{
    //header.setValid(false);

    if (0 > socket)
        return -1;

    //размер оставшегося буффера
    size_t size = m_sizeBuffer-m_pos;
    char* buff = m_buffer + m_pos;


    //на всякий случай MSG_NOSIGNAL, что б не упасть если та сторона неожиданно исчезнет
    int res  = recv(socket,buff, size, MSG_NOSIGNAL);

    //shutdown by client
    if (!res)
    {
        TRACE() << "shutdown by client " << endl;
        return -1;
    }

    //errors
    if (0 > res)
    {
        TRACE() << errno;
        return -1;
    }

    m_pos += res;

    //пытаемся выпарсивать построчно, столько сколько возможно

    char* bufferEnd = m_buffer + m_pos;

    char* httpEnd = strstr(m_buffer,"\r\n\r\n");

    TRACE() << string(m_buffer) << res << endl;

    //return 0;

    if (httpEnd == NULL || httpEnd >= bufferEnd)
        return +1;

    char* start = m_buffer;

    char* end = strstr(start,"\r\n");

    if (end == NULL || end  >= bufferEnd)
        return -1;

    if (!parseRequestLine(string(start,end-start),header))
    {
        return -1;
    }

    start = end + 2;

    while (1)
    {
        char* end = strstr(start,"\r\n");

        if (end == NULL || end  >= bufferEnd)
            return -1;

        int size = end - start;

        if (!size)
            break;

        if (!parseHeaderLine(string(start,end-start),header))
        {
            return -1;
        }

        start = end + 2;
    }

    m_pos = 0;
    memset(m_buffer,0,m_sizeBuffer);

    return 0;

}

//заголовок и буффер с телом ответа - к бинарному ответу..
void HttpHandler::responseHeaderToString(HttpResponseHeader& headerResp,std::string& buffer)
{

    ostringstream responseStream;

    //статусная строка
    responseStream << "HTTP/" << headerResp.majorVersion() << '.' << headerResp.minorVersion() << ' ' << headerResp.status() << ' ' << headerResp.pharse() << "\r\n";

    const map<string, string>& headers =   headerResp.allHeaders();

    map<string, string>::const_iterator it =  headers.begin();

    //строки заголовков
    while ( headers.end() != it)
    {
        const pair<string,string>& head = (*it);

        responseStream << head.first << ':' << head.second << "\r\n";

        it++;
    }

    //разделитель заголовка и тела
    responseStream << "\r\n";

    buffer = responseStream.str();
}


//возвращает false  в случае TCP/IP ошибки
bool HttpHandler::sendResponse(int socket,HttpResponseHeader& headerResp,ByteArray& bodyResp)
{
    string str;

    responseHeaderToString(headerResp,str);

    str.append((const char*)bodyResp.data(),bodyResp.size());


    int size   = str.size();
    char* data = (char*)str.data();
    int count = 0;

    while (0 < size)
    {

        data += count;
        size -= count;

        count = send(socket,data,size,MSG_NOSIGNAL);

        if ( 0 > count)
        {
            TRACE() << errno;
            return false;
        }
    }

    return true;
}


//собственно обработка запроса..
void HttpHandler::processingRequest(const HttpRequestHeader& header,const ByteArray& body,HttpResponseHeader& headerResp,ByteArray& bodyResp)
{
    headerResp.setMajorVersion(m_majorVersion);
    headerResp.setMinorVersion(m_minorVersion);

    //string host = header.value("host");


//    if (!header.isValid())
//    {
//        headerResp.setStatus(400);
//        headerResp.setPharse("Bad reqest");
//        return;
//    }

    if (header.method() != "GET")
    {
        headerResp.setStatus(501);
        headerResp.setPharse("Not Implemented");
        return;
    }


    //берём path и пытаеся найти такой ресурс и отдать...

    string realPath = m_path + header.path();


    if (readAll(realPath,bodyResp))
    {
        TRACE() << "from file" << endl;
        headerResp.setStatus(200);
        headerResp.setPharse("OK");
    }
    //иначе страница не найдена
    else
    {
        TRACE() << "not found" << endl;
        headerResp.setStatus(404);
        headerResp.setPharse("Not found");
    }

    ostringstream out;

    out << bodyResp.size();

    headerResp.addValue("Content-length",out.str());

    m_keepAfterResponse = header.value("connection") == "keep-alive" && headerResp.status() == 200;

    //полагаю что весь мой контент такой..
    headerResp.addValue("Content-Type","text/html; charset=UTF-8");
    //headerResp.addValue("Content-Type","text/html");

    if (m_keepAfterResponse)
        headerResp.addValue("Connection","keep-alive");
    else
        headerResp.addValue("Connection","close");


//    //полагаю что весь мой контент такой..
//    headerResp.addValue("Content-Type","text/html; charset=UTF-8");
    //headerResp.addValue("host",host);
}

int HttpHandler::processing()
{

    HttpRequestHeader requestHeader;
    ByteArray requestBody;

    HttpResponseHeader responsetHeader;
    ByteArray responsetBody;

    int res = getRequest(m_socket,requestHeader);
    //ошибка на уровне TCP - отвечать нет смысла
    if (res)
    {
        TRACE() << "aaa " << res << endl;
        //shutdown(m_socket,SHUT_RDWR);
        return res;
    }

    //TRACE() << "succefly recv data" << endl;

    processingRequest(requestHeader,requestBody,responsetHeader,responsetBody);


    if (!sendResponse(m_socket,responsetHeader,responsetBody))
    {
        TRACE() << "bbb";
        return 0;
        //shutdown(m_socket,SHUT_RDWR);
    }


    //shutdown(m_socket,SHUT_RDWR);

//    if (m_keepAfterResponse)
//    {
//        TRACE() << "keep-alive" << endl;
//        return +1;
//    }

    return 0;
}

bool HttpHandler::readAll(const std::string& name,ByteArray& data)
{

    struct stat path_stat;
    stat(name.c_str(), &path_stat);

    if (!S_ISREG(path_stat.st_mode))
        return false;

    std::ifstream in(name.c_str(),std::ios::in|std::ios::binary);
    TRACE() << "readAll " << name << " " << endl;

    if (!in.is_open())
    {
        TRACE() << "Unable open file " << name.c_str() << endl;
        return false;
    }

    in.seekg(0,std::ios::end);
    int len = in.tellg();

    if (len <=0)
        return false;

    in.seekg(0,std::ios::beg);

    TRACE() << "len = " << len << endl;
    data.reserve(len);
    data.resize(len);
    in.read((char*)data.data(),len);
    in.close();

    return true;

}

bool HttpHandler::parseRequestLine(const std::string &str, HttpRequestHeader &header)
{

    //парсится СТРОГО в соответсвии со строкой из RFC
    //Request-Line = Method SP Request-URI SP HTTP-Version CRLF

    string::size_type ind = str.find(' ');

    if (string::npos == ind)
    {
        return false;
    }

    header.setMethod(str.substr(0,ind));

    ind++;


    string::size_type old_ind = ind;

    ind = str.find(' ',old_ind);

    if (string::npos == ind)
    {
        return false;
    }

    int temp = str.find('?',old_ind);

    if (temp != string::npos)
	ind = temp;

    header.setPath(str.substr(old_ind,ind-old_ind));

    ind++;
    old_ind = ind;

    string vers = str.substr(old_ind);

    if (vers.length() < 6 )
    {
        return false;
    }

    //static const string httpLine("HTTP/");

//    if (vers.substr(0,5) != httpLine)
//    {
//        header.setValid(false);

//        return false;
//    }

    //HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT

    vers = vers.substr(5);

    ind = vers.find('.');

    if (string::npos == ind)
    {
        return  false;
    }

    vers = vers.replace(ind,1," ");

    istringstream versstream(vers,istringstream::in);

    unsigned int major=0;
    unsigned int minor=0;
    //char delimiter = 0;

    versstream >> major  >> minor;

    header.setMajorVersion(major);
    header.setMinorVersion(minor);

    TRACE() << vers << endl << major  << minor;

    return true;
}

bool HttpHandler::parseHeaderLine(const std::string &str, HttpRequestHeader &header)
{
    string::size_type ind = str.find(':');

    if (string::npos == ind)
    {
        return false;
    }

    const string& key = str.substr(0,ind);

    string value = str.substr(ind+1);

    ind = value.find_first_not_of(' ');

    if (string::npos == ind)
    {
        return false;
    }

    string::size_type  old_ind = ind;

    ind = value.find_last_not_of(' ');

    if (string::npos == ind)
    {
        return false;
    }

    value = value.substr(old_ind,ind-old_ind+1);

    header.addValue(key,value);

    TRACE() << key << " " << value <<  endl;

    return true;
}
