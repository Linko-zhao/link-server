#ifndef __LINKO_SOCKET_STREAM_H__
#define __LINKO_SOCKET_STREAM_H__

#include "stream.h"
#include "socket.h"

namespace linko {

class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;

    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    // buffer: 待接收数据的内存
    // length: 待接收数据的内存长度
    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;

    // buffer: 待发送数据的内存
    // length: 待发送数据的内存长度
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;

    virtual void close() override;

    Socket::ptr getSocket() const { return m_socket; }
    bool isConnected() const;

private:
    Socket::ptr m_socket;
    // 是否主控
    bool m_owner;
};

}

#endif
