/*
 * 文件句柄管理类
 */
#ifndef __LINKO_FD_MANAGER_H__
#define __LINKO_FD_MANAGER_H__

#include <vector>
#include <memory>
#include "thread.h"
#include "singleton.h"

namespace linko {

/*
 * 文件句柄上下文类
 * 管理文件句柄类型(是否socket)
 */
class FdCtx : public std::enable_shared_from_this<FdCtx>  {
public:
    typedef std::shared_ptr<FdCtx> ptr;
    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
    bool isClosed() const { return m_isClosed; }
    bool close();

    void setUserNonblock(bool v) { m_userNonblock = v; }
    bool getUserNonblock() const { return m_userNonblock; }

    void setSysNonblock(bool v) { m_sysNonblock = v; }
    bool getSysNonblock() const { return m_sysNonblock; }

    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

private:
    bool m_isInit: 1;       // 是否初始化
    bool m_isSocket: 1;     // 是否socket
    bool m_sysNonblock: 1;  // 是否hook非阻塞
    bool m_userNonblock: 1; // 是否用户主动设置非阻塞
    bool m_isClosed: 1;     // 是否关闭
    int m_fd;               // 文件句柄
    uint64_t m_recvTimeout; // 读超时时间
    uint64_t m_sendTimeout; // 写超时时间

};

/*
 * 文件句柄管理类
 */
class FdManager {
public:
    typedef RWMutex RWMutexType;
    FdManager();

    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    // 文件句柄合集
    std::vector<FdCtx::ptr> m_datas;
};

// 单例模式
typedef Singleton<FdManager> FdMgr;

}

#endif
