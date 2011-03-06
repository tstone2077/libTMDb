/* The MIT License:

Copyright (c) 2008 Ivan Gagis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// (c) Ivan Gagis, 2008
// e-mail: igagis@gmail.com

// Description:
//          cross platfrom C++ Sockets wrapper
//

#include "sckt.hpp"

//system specific defines, typedefs and includes
#ifdef __WIN32__
#include <winsock2.h>
#include <windows.h>

typedef SOCKET T_Socket;
#define M_INVALID_SOCKET INVALID_SOCKET
#define M_SOCKET_ERROR SOCKET_ERROR
#define M_EINTR WSAEINTR
#define M_FD_SETSIZE FD_SETSIZE

#else //assume linux/unix

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
typedef int T_Socket;
#define M_INVALID_SOCKET (-1)
#define M_SOCKET_ERROR (-1)
#define M_EINTR EINTR
#define M_FD_SETSIZE FD_SETSIZE

#endif

using namespace sckt;

Library* Library::instance = 0;

inline static T_Socket& CastToSocket(sckt::Socket::SystemIndependentSocketHandle& s){
    M_SCKT_STATIC_ASSERT( sizeof(s) >= sizeof(T_Socket) )
    return *reinterpret_cast<T_Socket*>(&s);
};

inline static const T_Socket& CastToSocket(const sckt::Socket::SystemIndependentSocketHandle& s){
    return CastToSocket(const_cast<sckt::Socket::SystemIndependentSocketHandle&>(s));
};

//static
void Library::InitSockets()throw(sckt::Exc){
#ifdef __WIN32__
    WORD versionWanted = MAKEWORD(2,2);
    WSADATA wsaData;
    if(WSAStartup(versionWanted, &wsaData) != 0 )
        throw sckt::Exc("sdlw::InitSockets(): Winsock 2.2 initialization failed");
#else //assume linux/unix
    // SIGPIPE is generated when a remote socket is closed
    void (*handler)(int);
    handler = signal(SIGPIPE, SIG_IGN);
    if(handler != SIG_DFL)
        signal(SIGPIPE, handler);
#endif
};

//static
void Library::DeinitSockets(){
#ifdef __WIN32__
    // Clean up windows networking
    if(WSACleanup() == M_SOCKET_ERROR)
        if(WSAGetLastError() == WSAEINPROGRESS){
            WSACancelBlockingCall();
            WSACleanup();
        }
#else //assume linux/unix
    // Restore the SIGPIPE handler
    void (*handler)(int);
    handler = signal(SIGPIPE, SIG_DFL);
    if(handler != SIG_IGN)
        signal(SIGPIPE, handler);
#endif
};

IPAddress Library::GetHostByName(const char *hostName, u16 port)throw(sckt::Exc){
    if(!hostName)
        throw sckt::Exc("Sockets::GetHostByName(): pointer passed as argument is 0");
    
    IPAddress addr;
    addr.host = inet_addr(hostName);
    if(addr.host == INADDR_NONE){
        struct hostent *hp;
        hp = gethostbyname(hostName);
        if(hp)
            memcpy(&(addr.host), hp->h_addr, sizeof(addr.host)/* hp->h_length */);
        else
            throw sckt::Exc("Sockets::GetHostByName(): gethostbyname() failed");
    }
    addr.port = port;
    return addr;
};

sckt::Exc::Exc(const char* message) throw(std::bad_alloc){
    if(message==0)
        message = "unknown exception";
    
    int len = strlen(message);
    this->msg = new char[len+1];
    memcpy(this->msg, message, len);
    this->msg[len] = 0;//null-terminate
};

sckt::Exc::~Exc()throw(){
    delete[] this->msg;
};

Library::Library()throw(sckt::Exc){
    if(Library::instance != 0)
        throw sckt::Exc("Library::Library(): sckt::Library singletone object is already created");
    Library::InitSockets();
    this->instance = this;
};

Library::~Library(){
    //this->instance should not be null here
    Library::DeinitSockets();
    this->instance = 0;
};

Socket::Socket() :
        isReady(false)
{
    CastToSocket(this->socket) = M_INVALID_SOCKET;
};

bool Socket::IsValid()const{
    return CastToSocket(this->socket) != M_INVALID_SOCKET;
};

Socket& Socket::operator=(const Socket& s){
    this->~Socket();
    CastToSocket(this->socket) = CastToSocket(s.socket);
    this->isReady = s.isReady;
    CastToSocket( const_cast<Socket&>(s).socket ) = M_INVALID_SOCKET;//same as std::auto_ptr
    return *this;
};

IPAddress::IPAddress() :
        host(INADDR_ANY),
        port(0)
{};

//static
sckt::u32 IPAddress::ParseString(const char* ip) throw(sckt::Exc){
    if(!ip)
        throw sckt::Exc("IPAddress::ParseString(): pointer passed as argument is 0");
    
    //subfunctions
    struct sf{
        static void ThrowInvalidIP(){
            throw sckt::Exc("IPAddress::ParseString(): string is not a valid IP address");
        };
    };
    
    u32 h = 0;//parsed host
    const char *curp = ip;
    for(uint t=0; t<4; ++t){
        uint dights[3];
        uint numDgts;
        for(numDgts=0; numDgts<3; ++numDgts){
            if( *curp == '.' || *curp == 0 ){
                if(numDgts==0)
                    sf::ThrowInvalidIP();
                break;
            }else{
                if(*curp < '0' || *curp > '9')
                    sf::ThrowInvalidIP();
                dights[numDgts] = uint(*curp)-uint('0');
            }
            ++curp;
        }
        
        if(t<3 && *curp != '.')//unexpected delimiter or unexpected end of string
            sf::ThrowInvalidIP();
        else if(t==3 && *curp != 0)
            sf::ThrowInvalidIP();
        
        uint xxx=0;
        for(uint i=0; i<numDgts; ++i){
            uint ord = 1;
            for(uint j=1; j<numDgts-i; ++j)
               ord*=10;
            xxx+=dights[i]*ord;
        }
        if(xxx > 255)
            sf::ThrowInvalidIP();
        
        h |= (xxx<<(8*t));
        
        ++curp;
    }
    return h;
};

/* Open a TCP network server socket
   This creates a local server socket on the given port.
*/
void TCPServerSocket::Open(u16 port, bool disableNaggle) throw(sckt::Exc){
    if(this->IsValid())
        throw sckt::Exc("TCPServerSocket::Open(): socket already opened");
    
    this->disableNaggle = disableNaggle;
    
    CastToSocket(this->socket) = ::socket(AF_INET, SOCK_STREAM, 0);
    if(CastToSocket(this->socket) == M_INVALID_SOCKET)
        throw sckt::Exc("TCPServerSocket::Open(): Couldn't create socket");
    
    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(port);

    // allow local address reuse
    {
        int yes = 1;
        setsockopt(CastToSocket(this->socket), SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
    }

    // Bind the socket for listening
    if( bind(CastToSocket(this->socket), reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockAddr)) == M_SOCKET_ERROR ){
        this->Close();
        throw sckt::Exc("TCPServerSocket::Open(): Couldn't bind to local port");
    }

    if( listen(CastToSocket(this->socket), 5) == M_SOCKET_ERROR ){
        this->Close();
        throw sckt::Exc("TCPServerSocket::Open(): Couldn't listen to local port");
    }

    //Set the socket to non-blocking mode for accept()
#if defined(__BEOS__) && defined(SO_NONBLOCK)
    // On BeOS r5 there is O_NONBLOCK but it's for files only
    {
        long b = 1;
        setsockopt(CastToSocket(this->socket), SOL_SOCKET, SO_NONBLOCK, &b, sizeof(b));
    }
#elif defined(O_NONBLOCK)
    {
        fcntl(CastToSocket(this->socket), F_SETFL, O_NONBLOCK);
    }
#elif defined(WIN32)
    {
        u_long mode = 1;
        ioctlsocket(CastToSocket(this->socket), FIONBIO, &mode);
    }
#elif defined(__OS2__)
    {
        int dontblock = 1;
        ioctl(CastToSocket(this->socket), FIONBIO, &dontblock);
    }
#else
#warning How do we set non-blocking mode on other operating systems?
#endif
};

/* Open a TCP network socket.
   A TCP connection to the remote host and port is attempted.
*/
void TCPSocket::Open(const IPAddress& ip, bool disableNaggle) throw(sckt::Exc){
    if(this->IsValid())
        throw sckt::Exc("TCPSocket::Open(): socket already opened");
    
    CastToSocket(this->socket) = ::socket(AF_INET, SOCK_STREAM, 0);
    if(CastToSocket(this->socket) == M_INVALID_SOCKET)
        throw sckt::Exc("TCPSocket::Open(): Couldn't create socket");
    
    //Connecting to remote host
    
    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = ip.host;
    sockAddr.sin_port = htons(ip.port);

    // Connect to the remote host
    if( connect(CastToSocket(this->socket), reinterpret_cast<sockaddr *>(&sockAddr), sizeof(sockAddr)) == M_SOCKET_ERROR ){
        this->Close();
        throw sckt::Exc("TCPSocket::Open(): Couldn't connect to remote host");
    }
    
    //Disable Naggle algorithm if required
    if(disableNaggle)
        this->DisableNaggle();
    
    this->isReady = false;
};

void TCPSocket::DisableNaggle() throw(sckt::Exc){
    if(!this->IsValid())
        throw sckt::Exc("TCPSocket::DisableNaggle(): socket is not opened");
    
    int yes = 1;
    setsockopt(CastToSocket(this->socket), IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(yes));
};

void Socket::Close(){
    if(this->IsValid()){
#ifdef __WIN32__
        //Closing socket in Win32.
        //refer to http://tangentsoft.net/wskfaq/newbie.html#howclose for details
        shutdown(CastToSocket(this->socket), SD_BOTH);
        closesocket(CastToSocket(this->socket));
#else //assume linux/unix
        close(CastToSocket(this->socket));
#endif
    }
    this->isReady = false;
    CastToSocket(this->socket) = M_INVALID_SOCKET;
};


TCPSocket TCPServerSocket::Accept() throw(sckt::Exc){
    if(!this->IsValid())
        throw sckt::Exc("TCPServerSocket::Accept(): the socket is not opened");
    
    this->isReady = false;
    
    sockaddr_in sockAddr;
    
#ifdef __WIN32__
    int sock_alen = sizeof(sockAddr);
#else //linux/unix
    socklen_t sock_alen = sizeof(sockAddr);
#endif
    
    TCPSocket sock;//allocate a new socket object
    
    CastToSocket(sock.socket) = accept(CastToSocket(this->socket), reinterpret_cast<sockaddr*>(&sockAddr),
#ifdef USE_GUSI_SOCKETS
                (unsigned int *)&sock_alen);
#else
                &sock_alen);
#endif
    
    if(CastToSocket(sock.socket) == M_INVALID_SOCKET)
        return sock;//no connections to be accepted, return invalid socket
    
    //set blocking mode
#ifdef __WIN32__
    {
        /* passing a zero value, socket mode set to block on */
        u_long mode = 0;
        ioctlsocket(CastToSocket(sock.socket), FIONBIO, &mode);
    }
#elif defined(O_NONBLOCK)
    {
        int flags = fcntl(CastToSocket(sock.socket), F_GETFL, 0);
        fcntl(CastToSocket(sock.socket), F_SETFL, flags & ~O_NONBLOCK);
    }
#else
#error do not know how to set blocking mode to socket
#endif //#ifdef WIN32
    
    if(this->disableNaggle)
        sock.DisableNaggle();
    
    return sock;//return a newly created socket
};


sckt::uint TCPSocket::Send(const sckt::byte* data, uint size) throw(sckt::Exc){
    if(!this->IsValid())
        throw sckt::Exc("TCPSocket::Send(): socket is not opened");
    
    int sent = 0,
        left = int(size);
    
    //Keep sending data until it's sent or an error occurs
    int errorCode = 0;

    int res;
    do{
        res = send(CastToSocket(this->socket), reinterpret_cast<const char*>(data), left, 0);
        if(res == M_SOCKET_ERROR){
#ifdef __WIN32__
            errorCode = WSAGetLastError();
#else //linux/unix
            errorCode = errno;
#endif
        }else{
            sent += res;
            left -= res;
            data += res;
        }
    }while( (left > 0) && ((res != M_SOCKET_ERROR) || (errorCode == M_EINTR)) );
    
    if(res == M_SOCKET_ERROR)
        throw sckt::Exc("TCPSocket::Send(): send() failed");
    
    return uint(sent);
};


sckt::uint TCPSocket::Recv(sckt::byte* buf, uint maxSize) throw(sckt::Exc){
    //this flag shall be cleared even if this function fails to avoid subsequent
    //calls to Recv() because it indicates that there's activity.
    //So, do it at the beginning of the function.
    this->isReady = false;
    
    if(!this->IsValid())
        throw sckt::Exc("TCPSocket::Send(): socket is not opened");
    
    int len;
    int errorCode = 0;
    
    do{
        len = recv(CastToSocket(this->socket), reinterpret_cast<char *>(buf), maxSize, 0);
        if(len == M_SOCKET_ERROR){
#ifdef __WIN32__
            errorCode = WSAGetLastError();
#else //linux/unix
            errorCode = errno;
#endif
        }
    }while(errorCode == M_EINTR);
    
    if(len == M_SOCKET_ERROR)
        throw sckt::Exc("TCPSocket::Recv(): recv() failed");
    
    return uint(len);
};

void UDPSocket::Open(u16 port) throw(sckt::Exc){
    if(this->IsValid())
        throw sckt::Exc("UDPSocket::Open(): the socket is already opened");
    
    CastToSocket(this->socket) = ::socket(AF_INET, SOCK_DGRAM, 0);
    if(CastToSocket(this->socket) == M_INVALID_SOCKET)
	throw sckt::Exc("UDPSocket::Open(): ::socket() failed");
    
    /* Bind locally, if appropriate */
    if(port != 0){
        struct sockaddr_in sockAddr;
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sin_family = AF_INET;
        sockAddr.sin_addr.s_addr = INADDR_ANY;
        sockAddr.sin_port = htons(port);
        
        // Bind the socket for listening
        if(bind(CastToSocket(this->socket), reinterpret_cast<struct sockaddr*>(&sockAddr), sizeof(sockAddr)) == M_SOCKET_ERROR){
            this->Close();
            throw sckt::Exc("UDPSocket::Open(): could not bind to local port");
        }
    }
#ifdef SO_BROADCAST
    //Allow LAN broadcasts with the socket
    {
        int yes = 1;
        setsockopt(CastToSocket(this->socket), SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
    }
#endif
    
    this->isReady = false;
};

sckt::uint UDPSocket::Send(const sckt::byte* buf, u16 size, IPAddress destinationIP) throw(sckt::Exc){
    sockaddr_in sockAddr;
    int sockLen = sizeof(sockAddr);
    
    sockAddr.sin_addr.s_addr = destinationIP.host;
    sockAddr.sin_port = htons(destinationIP.port);
    sockAddr.sin_family = AF_INET;
    int res = sendto(CastToSocket(this->socket), reinterpret_cast<const char*>(buf), size, 0, reinterpret_cast<struct sockaddr*>(&sockAddr), sockLen);
    
    if(res == M_SOCKET_ERROR)
        throw sckt::Exc("UDPSocket::Send(): sendto() failed");
    
    return res;
};

sckt::uint UDPSocket::Recv(sckt::byte* buf, u16 maxSize, IPAddress &out_SenderIP) throw(sckt::Exc){
    sockaddr_in sockAddr;
    
#ifdef __WIN32__
    int sockLen = sizeof(sockAddr);
#else //linux/unix
    socklen_t sockLen = sizeof(sockAddr);
#endif
    
    int res = recvfrom(CastToSocket(this->socket), reinterpret_cast<char*>(buf), maxSize, 0, reinterpret_cast<sockaddr*>(&sockAddr), &sockLen);
    
    if(res == M_SOCKET_ERROR)
        throw sckt::Exc("UDPSocket::Recv(): recvfrom() failed");
    
    out_SenderIP.host = ntohl(sockAddr.sin_addr.s_addr);
    out_SenderIP.port = ntohs(sockAddr.sin_port);
    return res;
};

SocketSet::SocketSet(uint maxNumSocks) throw(sckt::Exc, std::bad_alloc):
        maxSockets(maxNumSocks),
        numSockets(0)
{
    if(this->maxSockets > M_FD_SETSIZE)
        throw sckt::Exc("SocketSet::SocketSet(): socket size reuqested is too large");
    this->set = new Socket*[this->maxSockets];
};

void SocketSet::AddSocket(Socket *sock) throw(sckt::Exc){
    if(!sock)
        throw sckt::Exc("SocketSet::AddSocket(): null socket pointer passed as argument");
    
    if(this->numSockets == this->maxSockets)
        throw sckt::Exc("SocketSet::AddSocket(): socket set is full");
    
    for(uint i=0; i<this->numSockets; ++i){
        if(this->set[i] == sock)
            return;
    }
    
    this->set[this->numSockets] = sock;
    ++this->numSockets;
};

void SocketSet::RemoveSocket(Socket *sock) throw(sckt::Exc){
    if(!sock)
        throw sckt::Exc("SocketSet::RemoveSocket(): null socket pointer passed as argument");
    
    uint i;
    for(i=0; i<this->numSockets; ++i)
        if(this->set[i]==sock)
            break;
    
    if(i==this->numSockets)
        return;//socket sock not found in the set

    --this->numSockets;//decrease numsockets before shifting the sockets
    //shift sockets
    for(;i<this->numSockets; ++i){
        this->set[i] = this->set[i+1];
    }
};

bool SocketSet::CheckSockets(uint timeoutMillis){
    if(this->numSockets == 0)
        return false;
    
    T_Socket maxfd = 0;
    
    //Find the largest file descriptor
    for(uint i = 0; i<this->numSockets; ++i){
        if(CastToSocket(this->set[i]->socket) > maxfd)
            maxfd = CastToSocket(this->set[i]->socket);
    }
    
    int retval;
    fd_set readMask;
    
    //Check the file descriptors for available data
    int errorCode = 0;
    do{
        //Set up the mask of file descriptors
        FD_ZERO(&readMask);
        for(uint i=0; i<this->numSockets; ++i){
            T_Socket socketHnd = CastToSocket(this->set[i]->socket);
            FD_SET(socketHnd, &readMask);
        }
        
        // Set up the timeout
        //TODO: consider moving this out of do{}while() loop
        timeval tv;
        tv.tv_sec = timeoutMillis/1000;
        tv.tv_usec = (timeoutMillis%1000)*1000;
        
        retval = select(maxfd+1, &readMask, NULL, NULL, &tv);
        if(retval == M_SOCKET_ERROR){
#ifdef __WIN32__
            errorCode = WSAGetLastError();
#else
            errorCode = errno;
#endif
        }
    }while(errorCode == M_EINTR);
    
    // Mark all file descriptors ready that have data available
    if(retval != 0 && retval != M_SOCKET_ERROR){
        int numSocketsReady = 0;
        for(uint i=0; i<this->numSockets; ++i){
            T_Socket socketHnd = CastToSocket(this->set[i]->socket);
            if( (FD_ISSET(socketHnd, &readMask)) ){
                this->set[i]->isReady = true;
                ++numSocketsReady;
            }
        }
        
        //on Win32 when compiling with mingw there are some strange things,
        //sometimes retval is not zero but there is no any sockets marked as ready in readMask.
        //I do not know why this happens on win32 and mingw. The workaround is to calculate number
        //of active sockets mnually, ignoring the retval value.
        if(numSocketsReady > 0)
            return true;
    }
    return false;
};
