#include "httpheader.h"
#include <algorithm>

using namespace std;

const string & HttpRequestHeader::method() const
{
    return m_method;
}

void HttpRequestHeader::setMethod(const string &method)
{
    m_method = method;
}

unsigned int HttpHeader::majorVersion() const
{
    return m_major;
}

void HttpHeader::setMajorVersion(unsigned int vers)
{
    m_major = vers;
}

unsigned int HttpHeader::minorVersion() const
{
    return m_minor;
}

void HttpHeader::setMinorVersion(unsigned int vers)
{
    m_minor = vers;
}

void HttpHeader::addValue(const string &key, const string &value)
{
    //для регистронезависимости
    string realKey=key;
    transform(realKey.begin(),realKey.end(),realKey.begin(),::tolower);

    m_values.insert(make_pair(realKey,value));
}

void HttpHeader::removeValue(const string &key)
{
    //для регистронезависимости
    string realKey=key;
    transform(realKey.begin(),realKey.end(),realKey.begin(),::tolower);

    m_values.erase(realKey);
}

const string & HttpHeader::value(const string &key) const
{
    static const string emptyStr;

    //для регистронезависимости
    string realKey=key;
    transform(realKey.begin(),realKey.end(),realKey.begin(),::tolower);

     map<string,string>::const_iterator it =  m_values.find(realKey);

     if (m_values.end() != it)
         return (*it).second;
     else
         return emptyStr;
}

const map<string, string> & HttpHeader::allHeaders() const
{
    return m_values;
}

HttpHeader::HttpHeader()
{
}

//bool HttpHeader::isValid() const
//{
//    return m_valid;
//}

//void HttpHeader::setValid(bool valid)
//{
//    m_valid = valid;
//}

const string & HttpRequestHeader::path() const
{
    return m_path;
}

void HttpRequestHeader::setPath(const string &path)
{
    m_path = path;
}

HttpRequestHeader::HttpRequestHeader():HttpHeader()
{
    //setValid(false);
}

HttpResponseHeader::HttpResponseHeader():HttpHeader()
{
    //setValid(true);
}

const string & HttpResponseHeader::pharse() const
{
    return m_pharse;
}

void HttpResponseHeader::setPharse(const string &str)
{
    m_pharse = str;
}

unsigned int HttpResponseHeader::status() const
{
    return m_status;
}

void HttpResponseHeader::setStatus(unsigned int status)
{
    m_status = status;
}


