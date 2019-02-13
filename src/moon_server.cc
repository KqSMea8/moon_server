/***************************************************************************
 * 
 * Copyright (c) 2014 liudong9183@qq.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file bus_server.cc
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/05/29 16:56:20
 * @version $Revision$
 * @brief 
 *  
 **/
#include "moon_server.h"

namespace moon {
static const char *infomsg[] = {"unknown command",
    "empty command",
    "ok",
    "start trans fail",
    "get match info fail"};
uint64_t moon_client_t::client_ct = 0;

int32_t moon_client_t::read_socket()
{
    int64_t sz = read(this->fd, inbuf, sizeof(inbuf));
    if(sz == -1)
    {
        g_logger.error("read data from ip=%s, port=%d fd=%d, fail,error:%s", ip, port, fd, strerror(errno));
        return -1;
    }else if(sz == 0)
    {
        g_logger.debug("read socket eof from ip=%s, port=%d fd=%d, peer close fd", ip, port, fd);
        return 1;
    }

    this->inbuf[sz] = '\0';
    this->querybuf.append(this->inbuf);
    return 0;
}

int32_t moon_client_t::process_redis_protocol()
{
    std::string::size_type newline = this->querybuf.find_first_of("\r\n");
    if(newline == std::string::npos)
    {
        return 1;
    }
    //读取参数个数
    std::string str_argc = this->querybuf.substr(1, newline - 1);
    if (!str2num(str_argc.c_str(), argc)) {
        g_logger.error("redis protocl error, argument number=%s is invalid", str_argc.c_str());
        return -1;
    }
    //逐个参数进行读取
    for(int i = 0; i< argc; i++)
    {
        std::string::size_type pos1 = this->querybuf.find_first_of("\r\n", newline + 2);
        if (pos1 == std::string::npos)
        {
            this->argc = 0;
            this->argv.clear();
            return 1;
        }
        if (pos1 <= newline + 2 || this->querybuf[newline + 2] != '$')
        {
            return -1;
        }
        std::string str_argv_len = this->querybuf.substr(newline + 2, pos1 - newline - 2);
        std::string::size_type pos2 = this->querybuf.find_first_of("\r\n", pos1 + 2);
        if (pos2 == std::string::npos)
        {
            this->argc = 0;
            this->argv.clear();
            return 1;
        }

        if (pos2 <= pos1 + 2)
        {
            return -1;
        }
        std::string str_argv = this->querybuf.substr(pos1 + 2, pos2 - pos1 - 2);
        this->argv.push_back(str_argv);
        newline = pos2;
    }

    this->querybuf.erase(0, newline + 2);
    return 0;
}
int32_t moon_client_t::process_simple_protocol()
{
    std::string::size_type newline = this->querybuf.find_first_of("\r\n");
    if(newline == std::string::npos)
    {
        return 1;
    }
    std::string query = this->querybuf.substr(0, newline);
    g_logger.debug("query=%s, buffer=%s", query.c_str(), this->querybuf.c_str());
    //querybuf pos move
    this->querybuf.erase(0, newline + 2);

    //parse argment
    this->argc = 0;
    this->argv.clear();
    char delimiter = ' ';
    std::string::size_type begin = query.find_first_not_of(delimiter);
    while(begin != std::string::npos)
    {
        std::string::size_type end = query.find_first_of(delimiter, begin);
        if(end == std::string::npos)
        {
            std::string parm = query.substr(begin);
            this->argv.push_back(parm);
            this->argc++;
            return 0;
        }
        std::string parm = query.substr(begin, end - begin);
        this->argv.push_back(parm);
        this->argc++;
        begin = query.find_first_not_of(delimiter, end + 1);
    }

    return 0;
}
int32_t moon_client_t::write_socket()
{
    size_t len = strlen(this->outbuf);
    if (len != 0) {
        ssize_t sz = write(this->fd, outbuf, len);
        if(sz <= 0 || sz != (ssize_t)len)
        {
            g_logger.error("write data ip=%s, port=%d, fd=%d, fail,error:%s",
                    ip, port, fd, strerror(errno));
            return -1;
        }
    }
    len = replybuf.size();
    if (len != 0) {
        ssize_t sz = write(this->fd, replybuf.c_str(), len);
        if(sz <= 0 || sz != (ssize_t)len)
        {
            g_logger.error("write data ip=%s, port=%d, fd=%d, fail,error:%s",
                    ip, port, fd, strerror(errno));
            return -1;
        }
    }
    this->outbuf[0] = '\0';
    this->replybuf.clear();
    return 0;
}
/********************************************************************/
moon_server_t::moon_server_t():_stop_flag(false), _cron_count(0), _accept_client(NULL), _port(-1), _epollfd(-1) {
    bzero(_events, sizeof(struct epoll_event) * MAX_EPOLL_SIZE);
}

bool moon_server_t::init_server() {
    return true;
}

moon_server_t::~moon_server_t() {
    while (!_clients.empty()) {
        moon_client_t *myclient = _clients.back();
        _clients.pop_back();

        this->_del_event(myclient, myclient->mask);
        delete myclient;
    }

    if (this->_accept_client != NULL) {
        this->_del_event(this->_accept_client, this->_accept_client->mask);
        delete this->_accept_client;
        this->_accept_client = NULL;
    }


    if(this->_epollfd != -1)
    {
        close(this->_epollfd);
        this->_epollfd = -1;
    }
}

void moon_server_t::_free_client(moon_client_t *c)
{
    assert(c != NULL);
    //int index = c->fd;
    this->_del_event(c, c->mask);
    //assert(_client_list[index] != NULL);
    //delete _client_list[index];
    //_client_list[index] = NULL;
    std::list<moon_client_t*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ) {
        if (*it == c) {
            it = _clients.erase(it);
        } else {
            ++it;
        }
    }
    delete c;
}
bool moon_server_t::_add_event(moon_client_t *myclient, int event)
{
    int op = myclient->mask == 0 ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    struct epoll_event e;
    e.events = myclient->mask | event;
    e.data.u64 = 0;
    e.data.ptr = myclient;
    //e.data.fd = myclient->fd;
    int ret = epoll_ctl(this->_epollfd, op, myclient->fd, &e);
    if(ret != 0)
    {
        g_logger.error("epoll op add/mod fail,error:%s", strerror(errno));
        return false;
    }
    myclient->mask = e.events;
    return true;
}

bool moon_server_t::_del_event(moon_client_t *myclient, int event)
{
    event = myclient->mask & ~event;
    int op = event == 0 ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    struct epoll_event e;
    e.events = event;
    e.data.u64 = 0;
    e.data.ptr = myclient;
    //e.data.fd = myclient->fd;
    int ret = epoll_ctl(this->_epollfd, op, myclient->fd, &e);
    if(ret != 0)
    {
        g_logger.error("epoll fd=%d del event=%d fail, error:%s", myclient->fd, event, strerror(errno));
        return false;
    }
    myclient->mask = e.events;
    return true;
}
bool moon_server_t::_add_reply(moon_client_t *c, const char *s)
{
    snprintf(c->outbuf, sizeof(c->outbuf), "+%s\r\n", s);
    return this->_add_event(c, EPOLLOUT);
}
bool moon_server_t::_add_reply_bulk(moon_client_t *c, const char *s)
{
    char tmpbuf[128];
    uint64_t len = strlen(s);
    snprintf(tmpbuf, sizeof(tmpbuf), "$%lu\r\n", len);
    c->replybuf.append(tmpbuf);
    c->replybuf.append(s);
    c->replybuf.append("\r\n");
    return this->_add_event(c, EPOLLOUT);
}

bool moon_server_t::_add_error(moon_client_t *c, const char *s)
{
    snprintf(c->outbuf, sizeof(c->outbuf), "-ERR %s\r\n", s);
    return this->_add_event(c, EPOLLOUT);
}

bool moon_server_t::_accept_client_conn()
{
    sockaddr client_addr;
    socklen_t client_addr_len = 0;

    int client_fd = accept(this->_accept_client->fd, (sockaddr*)&client_addr, &client_addr_len);
    if(client_fd == -1)
    {
        g_logger.error("accept client socket fail,error:%s", strerror(errno));
        return false;
    }
    
    sockaddr_in* client_addr_in = (sockaddr_in*)(&client_addr);
    char *ip = inet_ntoa(client_addr_in->sin_addr);
    int port = ntohs(client_addr_in->sin_port);
    g_logger.debug("connect with client %s:%d", ip, port);
    moon_client_t* myclient = new(std::nothrow) moon_client_t(ip, port, client_fd);
    if (myclient == NULL) oom_error();
    //this->_client_list[myclient->fd] = myclient;
    this->_clients.push_back(myclient);
    return this->_add_event(myclient, EPOLLIN);
}


void moon_server_t::_process_query(moon_client_t *myclient)
{
    assert(myclient != NULL);
    /* 从socket中读数据 */
    int32_t ret = myclient->read_socket();
    if(ret == -1)
    {
        g_logger.error("read socket fail,error:%s", strerror(errno));
        this->_free_client(myclient);
        return;
    } else if (ret == 1) {
        g_logger.debug("peer close socket", myclient->ip, myclient->port);
        this->_free_client(myclient);
        return;
    }
    
    myclient->interact_time = time(NULL);
    /* 将读到的数据按照协议进行解析，并进行处理 */
    _parse_cmd(myclient);
}
void moon_server_t::_parse_cmd(moon_client_t *myclient)
{
    int ret = -1;
    while(myclient->querybuf.size() != 0)
    {
        if(myclient->querybuf[0] == '*')
        {
            ret = myclient->process_redis_protocol();
        }  else {
            ret = myclient->process_simple_protocol();
        }

        if(ret == -1)
        {
            g_logger.error("process protocol error");
            this->_free_client(myclient);
            return;
        }else if(ret == 1)
        {
            return;
        } else if (ret == 0)
        {
            _process_cmd(myclient);
            myclient->reset_argv();
        }
    }
}

void moon_server_t::_close_idle_connection() {
    time_t now = time_t();
    std::list<moon_client_t*>::iterator it;
    for (it = _clients.begin(); it != _clients.end();) {
        moon_client_t *myclient = *it;
        if (now - myclient->interact_time > MAX_IDLE_TIME) {
            g_logger.notice("client ip=%s, port=%d exceed max idle time, close",
                    myclient->ip, myclient->port);
            _free_client(myclient);
            it = _clients.erase(it);
        } else {
            ++it;
        }
    }
}

void moon_server_t::server_cron()
{
    ++_cron_count;
    if (!(_cron_count % 36000))
    {
        _close_idle_connection();
    }

}
void moon_server_t::make_info_msg(char *info_str, size_t len) {
        snprintf(info_str, len,
                "meta_ip:%s\r\n"
                "meta_port:%d\r\n"
                "server_id:%u\r\n",
                g_config.meta_ip,
                g_config.meta_port,
                g_config.server_id);
}
void moon_server_t::make_error_msg(char *info_str, size_t len) {
        snprintf(info_str, len,
                "meta_ip:%s\r\n"
                "meta_port:%d\r\n"
                "server_id:%u\r\n"
                "error1:%s\r\n"
                "error2:%s\r\n"
                "error3:%s\r\n"
                "error4:%s\r\n"
                "error5:%s\r\n",
                g_config.meta_ip,
                g_config.meta_port,
                g_config.server_id,
                g_logger.error_info1,
                g_logger.error_info2,
                g_logger.error_info3,
                g_logger.error_info4,
                g_logger.error_info5);
}
void moon_server_t::_process_cmd(moon_client_t *myclient)
{
    int ret;
    if(myclient->argc == 0)
    {
        g_logger.debug("empty command");
        this->_add_error(myclient, infomsg[REPLY_EMPTY_COMMAND]);
        return;
    }

    if(myclient->argc == 1 && myclient->argv[0] == "ping") {
        this->_add_reply(myclient, "pong");
    } else if (myclient->argc == 1 && myclient->argv[0] == "shutserver") {
        myclient->flag |= MOON_SHUTDOWN;
        this->_add_reply(myclient, infomsg[REPLY_OK]);
    } else if (myclient->argc == 1 && myclient->argv[0] == "quit") {
        myclient->flag |= MOON_QUIT_CLIENT;
        this->_add_reply(myclient, infomsg[REPLY_OK]);
    } else if (myclient->argc == 1 &&
            (myclient->argv[0] == "info" || myclient->argv[0] == "INFO")) {
        char info_str[1024];
        make_info_msg(info_str, sizeof(info_str));
        this->_add_reply_bulk(myclient, info_str);
    }  else if (myclient->argc == 2 && 
            myclient->argv[0] == "info" && myclient->argv[1] == "error") {
        char info_str[1024];
        make_error_msg(info_str, sizeof(info_str));
        this->_add_reply_bulk(myclient, info_str);
    } else if(myclient->argc == 3 &&
            myclient->argv[0] == "set" && myclient->argv[1] == "loglevel") {
        g_logger.notice("recv set loglevel command");
        int32_t ret = 0;
        if (myclient->argv[2] == "debug") {
            ret = LOG_DEBUG;
        } else if (myclient->argv[2] == "info") {
            ret = LOG_VERBOSE;
        } else if (myclient->argv[2] == "notice") {
            ret = LOG_NOTICE;
        } else if (myclient->argv[2] == "warn") {
            ret = LOG_WARNING;
        } else if (myclient->argv[2] == "error") {
            ret = LOG_ERROR;
        } else {
            this->_add_error(myclient, "loglevel is invalid");
            return;
        }
        g_logger.set_loglevel(ret);
        this->_add_reply(myclient, infomsg[REPLY_OK]);
    } else {
        g_logger.notice("command=%s,unknown command", myclient->argv[0].c_str());
        this->_add_error(myclient, infomsg[REPLY_UNKNOWN_COMMAND]);
    }
}

void moon_server_t::_process_response(moon_client_t *myclient)
{
    assert(myclient != NULL);
    int ret = myclient->write_socket();
    if(ret != 0)
    {
        g_logger.error("write data fail");
        this->_free_client(myclient);
        return;
    }

    this->_del_event(myclient, EPOLLOUT);
    if (myclient->flag & MOON_SHUTDOWN) _stop_flag = true;
    if (myclient->flag & MOON_QUIT_CLIENT) {
        this->_free_client(myclient);
    }
}

void moon_server_t::start_server(int32_t port)
{
    int32_t acceptfd;
    this->_port = port;
    sockaddr_in server_addr;
    memset(&server_addr, '\0', sizeof(sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(this->_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //create accept socket
    acceptfd = socket(AF_INET, SOCK_STREAM, 0);
    if(acceptfd == -1){
        g_logger.error("create accept fd fail, error:%s", strerror(errno));
        fatal_error();
    }
    this->_accept_client = new(std::nothrow)moon_client_t("accept",-1, acceptfd);
    if (this->_accept_client == NULL) {
        g_logger.error("new accept client fail");
        fatal_error();
    }

    int mode = 1;
    int32_t ret = setsockopt(acceptfd, SOL_SOCKET, SO_REUSEADDR, (void*)&mode, sizeof(int));
    if (ret != 0)
    {
	g_logger.error("set socket resuse fail,error:%s", strerror(errno));
	fatal_error();
    }
    //bind address
    ret = bind(acceptfd, (sockaddr*)&server_addr, sizeof(server_addr));
    if(ret != 0){
        g_logger.error("bind address fail,error:%s", strerror(errno));
        fatal_error();
    }
    //create epoll fd
    this->_epollfd = epoll_create(MAX_EPOLL_SIZE);
    
    //add accept client to epoll
    this->_add_event(this->_accept_client, EPOLLIN);

    //listen accept fd
    ret = listen(acceptfd, 10);
    if(ret == -1){
        g_logger.error("listen accept fd fail,error:%s", strerror(errno));
        fatal_error();
    }
    
    g_logger.notice("start server port=%d succ", port);
    //begin accept a connection
    while(!_stop_flag)
    {
        int fdnum = epoll_wait(this->_epollfd, this->_events, sizeof(_events)/sizeof(struct epoll_event), LOOP_TIME);
        if (fdnum == -1)
        {
            if (errno != EINTR) {
                g_logger.error("epoll wait fail,error:%s", strerror(errno));
            }
            fdnum = 0;
        }

        for(int i = 0; i < fdnum; i++)
        {
            if(this->_events[i].data.ptr == this->_accept_client)
            {
                this->_accept_client_conn();
            } else if(this->_events[i].events & EPOLLIN)
            {
                //int fd = this->_events[i].data.fd;
                moon_client_t *myclient = (moon_client_t*)this->_events[i].data.ptr;

                _process_query(myclient);
            } else if(this->_events[i].events & EPOLLOUT) {
                //int fd = this->_events[i].data.fd;
                moon_client_t *myclient = (moon_client_t*)this->_events[i].data.ptr;
                _process_response(myclient);
            }
        }//for
        server_cron();
    }//while
    g_logger.notice("server is exiting...");
}
void moon_server_t::stop_server()
{
    g_logger.notice("server begin stop");
    _stop_flag = true;
}
moon_server_t g_server;

}//namespace moon
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
