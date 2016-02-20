#ifndef HTTPHEADER_H
#define HTTPHEADER_H

#include <map>
#include <string>


class HttpHeader
{
public:

    HttpHeader();

    //получить/установить версиии HTTP протокола
    unsigned int majorVersion() const;

    void setMajorVersion(unsigned int vers);

    unsigned int minorVersion() const;

    void setMinorVersion(unsigned int vers);


    //добавить, удалить, получить значение заголовка по имени, получить все заголовки
    void addValue(const std::string& key,const std::string& value);

    void removeValue(const std::string& key);

    const std::string& value(const std::string& key) const;

    const  std::map<std::string,std::string>& allHeaders() const;

    //bool isValid() const;

    //void setValid(bool valid);

private:
    unsigned int m_major;
    unsigned int m_minor;
    std::map<std::string,std::string> m_values;
    bool m_valid;

};

//класс хранилище данных http запроса
class HttpRequestHeader: public HttpHeader
{
  public:
    HttpRequestHeader();

    //получить/установить метод
    const std::string& method() const;

    void setMethod(const std::string& method);

    //получить/установить путь
    const std::string& path() const;

    void setPath(const std::string& path);

private:
    std::string m_method;
    std::string m_path;
};


class HttpResponseHeader: public HttpHeader
{
  public:
    HttpResponseHeader();

    //
    const std::string& pharse() const;
    void setPharse(const std::string& str);

    unsigned int status() const;
    void setStatus(unsigned int status);


private:
    std::string m_pharse;
    unsigned int m_status;

};


#endif // HTTPHEADER_H
