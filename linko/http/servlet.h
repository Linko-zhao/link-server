#ifndef __LINKO_HTTP_SERVLET_H__
#define __LINKO_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "../thread.h"

namespace linko {
namespace http {

class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name) : m_name(name) {}
    virtual ~Servlet() {}
    virtual int32_t handle(linko::http::HttpRequest::ptr request
            , linko::http::HttpResponse::ptr response
            , linko::http::HttpSession::ptr session) = 0;

    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    // 函数回调类型定义
    typedef std::function<int32_t (linko::http::HttpRequest::ptr request
            , linko::http::HttpResponse::ptr response
            , linko::http::HttpSession::ptr session)> callback;

    FunctionServlet(callback cb);
    virtual int32_t handle(linko::http::HttpRequest::ptr request
            , linko::http::HttpResponse::ptr response
            , linko::http::HttpSession::ptr session) override;

private:
    callback m_cb;
};

/*
 * Servlet分发器
 */
class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();
    virtual int32_t handle(linko::http::HttpRequest::ptr request
            , linko::http::HttpResponse::ptr response
            , linko::http::HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobalServlet(const std::string& uri);

    Servlet::ptr getMatchedServlet(const std::string& uri);

private:
    RWMutexType m_mutex;
    // 精准匹配
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // 模糊匹配
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet();
    virtual int32_t handle(linko::http::HttpRequest::ptr request
            , linko::http::HttpResponse::ptr response
            , linko::http::HttpSession::ptr session) override;
};

}
}

#endif
