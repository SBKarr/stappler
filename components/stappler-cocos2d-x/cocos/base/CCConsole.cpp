/****************************************************************************
 Copyright (c) 2013-2014 Chukong Technologies Inc.

 http://www.cocos2d-x.org

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
 THE SOFTWARE.
 ****************************************************************************/

#include "base/CCConsole.h"

#include <thread>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include <WS2tcpip.h>
#include <Winsock2.h>
#define bzero(a, b) memset(a, 0, b);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
#include "inet_ntop_winrt.h"
#include "inet_pton_winrt.h"
#include "CCWinRTUtils.h"
#endif
#else
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#endif

#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCPlatformConfig.h"
#include "base/CCConfiguration.h"
#include "2d/CCScene.h"
#include "platform/CCFileUtils.h"
#include "base/ccUtils.h"

NS_CC_BEGIN

extern const char* cocos2dVersion(void);

// helper free functions

// dprintf() is not defined in Android
// so we add our own 'dpritnf'
static ssize_t mydprintf(int sock, const char *format, ...)
{
    va_list args;
	char buf[16386];

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	return send(sock, buf, strlen(buf),0);
}

static void sendPrompt(int fd)
{
    const char prompt[] = "> ";
    send(fd, prompt, strlen(prompt),0);
}

//
// Free functions to log
//

static void _log(const char *format, va_list args)
{
    char buf[MAX_LOG_LENGTH];

    vsnprintf(buf, MAX_LOG_LENGTH-3, format, args);
    strcat(buf, "\n");

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    __android_log_print(ANDROID_LOG_DEBUG, "cocos2d-x debug info",  "%s", buf);

#elif CC_TARGET_PLATFORM ==  CC_PLATFORM_WIN32 || CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
    WCHAR wszBuf[MAX_LOG_LENGTH] = {0};
    MultiByteToWideChar(CP_UTF8, 0, buf, -1, wszBuf, sizeof(wszBuf));
    OutputDebugStringW(wszBuf);
    WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, buf, sizeof(buf), nullptr, FALSE);
    printf("%s", buf);
    fflush(stdout);
#else
    // Linux, Mac, iOS, etc
    fprintf(stdout, "%s", buf);
    fflush(stdout);
#endif

    Director::getInstance()->getConsole()->log(buf);

}

// FIXME: Deprecated
void CCLog(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    _log(format, args);
    va_end(args);
}

void log(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    _log(format, args);
    va_end(args);
}

//
// Console code
//

Console::Console()
: _listenfd(-1)
, _running(false)
, _endThread(false)
, _sendDebugStrings(false)
, _bindAddress("")
{
    // VS2012 doesn't support initializer list, so we create a new array and assign its elements to '_command'.
	Command commands[] = {
        { "allocator", "Display allocator diagnostics for all allocators", std::bind(&Console::commandAllocator, this, std::placeholders::_1, std::placeholders::_2) },
        { "config", "Print the Configuration object", std::bind(&Console::commandConfig, this, std::placeholders::_1, std::placeholders::_2) },
        { "debugmsg", "Whether or not to forward the debug messages on the console. Args: [on | off]", [&](int fd, const std::string& args) {
            if( args.compare("on")==0 || args.compare("off")==0) {
                _sendDebugStrings = (args.compare("on") == 0);
            } else {
                mydprintf(fd, "Debug message is: %s\n", _sendDebugStrings ? "on" : "off");
            }
        } },
        { "exit", "Close connection to the console", std::bind(&Console::commandExit, this, std::placeholders::_1, std::placeholders::_2) },
        { "help", "Print this message", std::bind(&Console::commandHelp, this, std::placeholders::_1, std::placeholders::_2) },
        { "projection", "Change or print the current projection. Args: [2d | 3d]", std::bind(&Console::commandProjection, this, std::placeholders::_1, std::placeholders::_2) },
        { "resolution", "Change or print the window resolution. Args: [width height resolution_policy | ]", std::bind(&Console::commandResolution, this, std::placeholders::_1, std::placeholders::_2) },
        { "director", "director commands, type -h or [director help] to list supported directives", std::bind(&Console::commandDirector, this, std::placeholders::_1, std::placeholders::_2) },
        { "version", "print version string ", [](int fd, const std::string& args) {
            mydprintf(fd, "%s\n", cocos2dVersion());
        } },
    };

	for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i)
	{
		_commands[commands[i].name] = commands[i];
	}
}

Console::~Console()
{
    stop();
}

bool Console::listenOnTCP(int port)
{
    int listenfd = -1, n;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;
    char serv[30];

    snprintf(serv, sizeof(serv)-1, "%d", port );

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET; // AF_UNSPEC: Do we need IPv6 ?
    hints.ai_socktype = SOCK_STREAM;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
    WSADATA wsaData;
    n = WSAStartup(MAKEWORD(2, 2),&wsaData);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8)
    CCLogIPAddresses();
#endif
#endif

    if ( (n = getaddrinfo(nullptr, serv, &hints, &res)) != 0) {
        // fprintf(stderr,"net_listen error for %s: %s", serv, gai_strerror(n));
        return false;
    }

    ressave = res;

    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0)
            continue;       /* error, try next one */

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

        // bind address
        if (_bindAddress.length() > 0)
        {
            if (res->ai_family == AF_INET)
            {
                struct sockaddr_in *sin = (struct sockaddr_in*) res->ai_addr;
                inet_pton(res->ai_family, _bindAddress.c_str(), (void*)&sin->sin_addr);
            }
            else if (res->ai_family == AF_INET6)
            {
                struct sockaddr_in6 *sin = (struct sockaddr_in6*) res->ai_addr;
                inet_pton(res->ai_family, _bindAddress.c_str(), (void*)&sin->sin6_addr);
            }
        }

        if (::bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            break;          /* success */

/* bind error, close and try next one */
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
        closesocket(listenfd);
#else
        close(listenfd);
#endif
    } while ( (res = res->ai_next) != nullptr);

    if (res == nullptr) {
        perror("net_listen:");
        freeaddrinfo(ressave);
        return false;
    }

    listen(listenfd, 50);

    if (res->ai_family == AF_INET) {
        char buf[INET_ADDRSTRLEN] = "";
        struct sockaddr_in *sin = (struct sockaddr_in*) res->ai_addr;
        if( inet_ntop(res->ai_family, &sin->sin_addr, buf, sizeof(buf)) != nullptr )
            cocos2d::log("Console: listening on  %s : %d", buf, ntohs(sin->sin_port));
        else
            perror("inet_ntop");
    } else if (res->ai_family == AF_INET6) {
        char buf[INET6_ADDRSTRLEN] = "";
        struct sockaddr_in6 *sin = (struct sockaddr_in6*) res->ai_addr;
        if( inet_ntop(res->ai_family, &sin->sin6_addr, buf, sizeof(buf)) != nullptr )
            cocos2d::log("Console: listening on  %s : %d", buf, ntohs(sin->sin6_port));
        else
            perror("inet_ntop");
    }


    freeaddrinfo(ressave);
    return listenOnFileDescriptor(listenfd);
}

bool Console::listenOnFileDescriptor(int fd)
{
    if(_running) {
        cocos2d::log("Console already started. 'stop' it before calling 'listen' again");
        return false;
    }

    _listenfd = fd;
    _thread = std::thread( std::bind( &Console::loop, this) );

    return true;
}

void Console::stop()
{
    if( _running ) {
        _endThread = true;
        _thread.join();
    }
}

void Console::addCommand(const Command& cmd)
{
    _commands[cmd.name]=cmd;
}

//
// commands
//

void Console::commandHelp(int fd, const std::string &args)
{
    const char help[] = "\nAvailable commands:\n";
    send(fd, help, sizeof(help),0);
    for(auto it=_commands.begin();it!=_commands.end();++it)
    {
        auto cmd = it->second;
        mydprintf(fd, "\t%s", cmd.name.c_str());
        ssize_t tabs = strlen(cmd.name.c_str()) / 8;
        tabs = 3 - tabs;
        for(int j=0;j<tabs;j++){
             mydprintf(fd, "\t");
        }
        mydprintf(fd,"%s\n", cmd.help.c_str());
    }
}

void Console::commandExit(int fd, const std::string &args)
{
    FD_CLR(fd, &_read_set);
    _fds.erase(std::remove(_fds.begin(), _fds.end(), fd), _fds.end());
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
        closesocket(fd);
#else
        close(fd);
#endif
}

void Console::commandConfig(int fd, const std::string& args)
{
    Scheduler *sched = Director::getInstance()->getScheduler();
    sched->performFunctionInCocosThread( [=](){
        mydprintf(fd, "%s", Configuration::getInstance()->getInfo().c_str());
        sendPrompt(fd);
    }
                                        );
}

void Console::commandResolution(int fd, const std::string& args)
{
    if(args.length()==0) {
        auto director = Director::getInstance();
        Size points = director->getWinSize();
        Size pixels = director->getWinSizeInPixels();
        auto glview = director->getOpenGLView();
        Size design = glview->getDesignResolutionSize();
        ResolutionPolicy res = glview->getResolutionPolicy();
        Rect visibleRect = glview->getVisibleRect();

        mydprintf(fd, "Window Size:\n"
                        "\t%d x %d (points)\n"
                        "\t%d x %d (pixels)\n"
                        "\t%d x %d (design resolution)\n"
                        "Resolution Policy: %d\n"
                        "Visible Rect:\n"
                        "\torigin: %d x %d\n"
                        "\tsize: %d x %d\n",
                  (int)points.width, (int)points.height,
                  (int)pixels.width, (int)pixels.height,
                  (int)design.width, (int)design.height,
                  (int)res,
                  (int)visibleRect.origin.x, (int)visibleRect.origin.y,
                  (int)visibleRect.size.width, (int)visibleRect.size.height
                  );

    } else {
        int width, height, policy;

        std::istringstream stream( args );
        stream >> width >> height>> policy;

        Scheduler *sched = Director::getInstance()->getScheduler();
        sched->performFunctionInCocosThread( [=](){
            Director::getInstance()->getOpenGLView()->setDesignResolutionSize(width, height, static_cast<ResolutionPolicy>(policy));
        } );
    }
}

void Console::commandProjection(int fd, const std::string& args)
{
    auto director = Director::getInstance();
    Scheduler *sched = director->getScheduler();

    if(args.length()==0)
    {
        char buf[20];
        auto proj = director->getProjection();
        switch (proj) {
            case cocos2d::Director::Projection::_2D:
                sprintf(buf,"2d");
                break;
            case cocos2d::Director::Projection::_3D:
                sprintf(buf,"3d");
                break;
            case cocos2d::Director::Projection::CUSTOM:
                sprintf(buf,"custom");
                break;

            default:
                sprintf(buf,"unknown");
                break;
        }
        mydprintf(fd, "Current projection: %s\n", buf);
    }
    else if( args.compare("2d") == 0)
    {
        sched->performFunctionInCocosThread( [=](){
            director->setProjection(Director::Projection::_2D);
        } );
    }
    else if( args.compare("3d") == 0)
    {
        sched->performFunctionInCocosThread( [=](){
            director->setProjection(Director::Projection::_3D);
        } );
    }
    else
    {
        mydprintf(fd, "Unsupported argument: '%s'. Supported arguments: '2d' or '3d'\n", args.c_str());
    }
}

void Console::commandDirector(int fd, const std::string& args)
{
     auto director = Director::getInstance();
    if(args =="help" || args == "-h")
    {
        const char help[] = "available director directives:\n"
                            "\tpause, pause all scheduled timers, the draw rate will be 4 FPS to reduce CPU consumption\n"
                            "\tend, exit this app.\n"
                            "\tresume, resume all scheduled timers\n"
                            "\tstop, Stops the animation. Nothing will be drawn.\n"
                            "\tstart, Restart the animation again, Call this function only if [director stop] was called earlier\n";
         send(fd, help, sizeof(help) - 1,0);
    }
    else if(args == "pause")
    {
        Scheduler *sched = director->getScheduler();
            sched->performFunctionInCocosThread( [](){
            Director::getInstance()->pause();
        }
                                        );

    }
    else if(args == "resume")
    {
        director->resume();
    }
    else if(args == "stop")
    {
        Scheduler *sched = director->getScheduler();
        sched->performFunctionInCocosThread( [](){
            Director::getInstance()->stopAnimation();
        }
                                        );
    }
    else if(args == "start")
    {
        director->startAnimation();
    }
    else if(args == "end")
    {
        director->end();
    }

}

void Console::commandAllocator(int fd, const std::string& args)
{
#if CC_ENABLE_ALLOCATOR_DIAGNOSTICS
    auto info = allocator::AllocatorDiagnostics::instance()->diagnostics();
    mydprintf(fd, info.c_str());
#else
    mydprintf(fd, "allocator diagnostics not available. CC_ENABLE_ALLOCATOR_DIAGNOSTICS must be set to 1 in ccConfig.h");
#endif
}

ssize_t Console::readBytes(int fd, char* buffer, size_t maxlen, bool* more)
{
    size_t n, rc;
    char c, *ptr = buffer;
    *more = false;
    for( n = 0; n < maxlen; n++ ) {
        if( (rc = recv(fd, &c, 1, 0)) ==1 ) {
            *ptr++ = c;
            if(c == '\n') {
                return n;
            }
        } else if( rc == 0 ) {
            return 0;
        } else if( errno == EINTR ) {
            continue;
        } else {
            return -1;
        }
    }
    *more = true;
    return n;
}

bool Console::parseCommand(int fd)
{
    return true;
}

//
// Helpers
//


ssize_t Console::readline(int fd, char* ptr, size_t maxlen)
{
    size_t n, rc;
    char c;

    for( n = 0; n < maxlen - 1; n++ ) {
        if( (rc = recv(fd, &c, 1, 0)) ==1 ) {
            *ptr++ = c;
            if(c == '\n') {
                break;
            }
        } else if( rc == 0 ) {
            return 0;
        } else if( errno == EINTR ) {
            continue;
        } else {
            return -1;
        }
    }

    *ptr = 0;
    return n;
}


void Console::addClient()
{
    struct sockaddr client;
    socklen_t client_len;

    /* new client */
    client_len = sizeof( client );
    int fd = accept(_listenfd, (struct sockaddr *)&client, &client_len );

    // add fd to list of FD
    if( fd != -1 ) {
        FD_SET(fd, &_read_set);
        _fds.push_back(fd);
        _maxfd = std::max(_maxfd,fd);

        sendPrompt(fd);
    }
}

void Console::log(const char* buf)
{
    if( _sendDebugStrings ) {
        _DebugStringsMutex.lock();
        _DebugStrings.push_back(buf);
        _DebugStringsMutex.unlock();
    }
}

//
// Main Loop
//
void Console::loop()
{
    fd_set copy_set;
    struct timeval timeout, timeout_copy;

    _running = true;

    FD_ZERO(&_read_set);
    FD_SET(_listenfd, &_read_set);
    _maxfd = _listenfd;

    timeout.tv_sec = 0;

    /* 0.016 seconds. Wake up once per frame at 60PFS */
    timeout.tv_usec = 16000;

    while(!_endThread) {

        copy_set = _read_set;
        timeout_copy = timeout;

        int nready = select(_maxfd+1, &copy_set, nullptr, nullptr, &timeout_copy);

        if( nready == -1 )
        {
            /* error */
            if(errno != EINTR)
                log("Abnormal error in select()\n");
            continue;
        }
        else if( nready == 0 )
        {
            /* timeout. do somethig ? */
        }
        else
        {
            /* new client */
            if(FD_ISSET(_listenfd, &copy_set)) {
                addClient();
                if(--nready <= 0)
                    continue;
            }

            /* data from client */
            std::vector<int> to_remove;
            for(const auto &fd: _fds) {
                if(FD_ISSET(fd,&copy_set))
                {
                    //fix Bug #4302 Test case ConsoleTest--ConsoleUploadFile crashed on Linux
                    //On linux, if you send data to a closed socket, the sending process will
                    //receive a SIGPIPE, which will cause linux system shutdown the sending process.
                    //Add this ioctl code to check if the socket has been closed by peer.

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
                    u_long n = 0;
                    ioctlsocket(fd, FIONREAD, &n);
#else
                    int n = 0;
                    ioctl(fd, FIONREAD, &n);
#endif
                    if(n == 0)
                    {
                        //no data received, or fd is closed
                        continue;
                    }

                    if( ! parseCommand(fd) )
                    {
                        to_remove.push_back(fd);
                    }
                    if(--nready <= 0)
                        break;
                }
            }

            /* remove closed conections */
            for(int fd: to_remove) {
                FD_CLR(fd, &_read_set);
                _fds.erase(std::remove(_fds.begin(), _fds.end(), fd), _fds.end());
            }
        }

        /* Any message for the remote console ? send it! */
        if( !_DebugStrings.empty() ) {
            _DebugStringsMutex.lock();
            for(const auto &str : _DebugStrings) {
                for(const auto &fd : _fds) {
                    send(fd, str.c_str(), str.length(),0);
                }
            }
            _DebugStrings.clear();
            _DebugStringsMutex.unlock();
        }
    }

    // clean up: ignore stdin, stdout and stderr
    for(const auto &fd: _fds )
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
        closesocket(fd);
#else
        close(fd);
#endif
    }

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
    closesocket(_listenfd);
	WSACleanup();
#else
    close(_listenfd);
#endif
    _running = false;
}

void Console::setBindAddress(const std::string &address)
{
    _bindAddress = address;
}

NS_CC_END
