#ifndef __LINKO_URI_H__
#define __LINKO_URI_H__

#include <memory>
#include <string>
#include "address.h"

namespace linko {

/*
*     foo://example.com:8042/over/there?name=ferret#nose
*     \_/   \______________/\_________/ \_________/ \__/
*      |           |            |            |        |
*   scheme     authority       path        query   fragment
*/

class Uri {
public:
    typedef std::shared_ptr<Uri> ptr;

    static Uri::ptr Create(const std::string& uri);
    Uri();

    const std::string& getScheme() const { return m_scheme; }
    void setScheme(const std::string& v) { m_scheme = v; }

    const std::string& getUserinfo() const { return m_userinfo; }
    void setUserinfo(const std::string& v) { m_userinfo = v; }

    const std::string& getHost() const { return m_host; }
    void setHost(const std::string& v) { m_host = v; }

    const std::string& getPath() const;
    void setPath(const std::string& v) { m_path = v; }

    const std::string& getQuery() const { return m_query; }
    void setQuery(const std::string& v) { m_query = v; }

    const std::string& getFragment() const { return m_fragment; }
    void setFragment(const std::string& v) { m_fragment = v; }

    int32_t getPort() const;
    void setPort(int32_t v) { m_port = v; }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
    Address::ptr createAddress() const;

private:
    bool isDefaultPort() const;
    
private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    int32_t m_port;
};

}

#endif
