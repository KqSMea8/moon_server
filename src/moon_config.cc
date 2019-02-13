/***************************************************************************
 * 
 * Copyright (c) 2018 liudong9183@qq.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file config.cc
 * @author liudong(liudong9183@qq.com)
 * @date 2018/11/28 15:55:14
 * @version $Revision$
 * @brief 
 *  
 **/

#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include "moon_config.h"

namespace moon {

moon_config_t::moon_config_t():cmd_port(-1), meta_port(-1) {
    cmd_dirpath[0] = '\0';
    meta_ip[0] = '\0';
    local_ip[0] = '\0';
    is_daemon = false;
}

bool moon_config_t::parse_cmd_arg(const int argc, char* const *argv) {
    const char *short_options = "p:D:H:P:d";
    char *endptr;
    struct option long_options[] = {
        {"port", 1, NULL, 'p'}, 
        {"dir", 1, NULL, 'D'},
        {"mhost", 1, NULL, 'H'},
        {"mport", 1, NULL, 'P'},
        {"daemon", 0, NULL, 'd'},
        {0, 0, 0, 0},
    };

    int c = 0;
    while((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (c) {
            case 'p':
                cmd_port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    g_logger.error("port=%s invalid", optarg);
                    return false;
                }
                break;
            case 'D':
                snprintf(cmd_dirpath, sizeof(cmd_dirpath), "%s", optarg);
                break;
            case 'H':
                snprintf(meta_ip, sizeof(meta_ip), "%s", optarg);
                break;
            case 'P':
                meta_port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    g_logger.error("meta port=%s invalid", optarg);
                    return false;
                }
                break;
            case 'd':
                is_daemon = true;
                break;
            default:
                g_logger.error("unknown option %c", c);
                return false;
        }
    }

    if (!check_cmd_arg()) {
        g_logger.error("check command args fail");
        return false;
    }

    return true;
}
bool moon_config_t::daemon() {
    int status;
    pid_t pid, sid;
    int fd;

    if (!is_daemon) {
        g_logger.error("server run in no daemon mode...");
        return true;
    }

    pid = fork();
    switch(pid) {
        case -1: 
            g_logger.error("1st fork fail, error:%s", strerror(errno));
            return false;
        case 0:
            break;
        default:
            _exit(0); //parent process exit
    }

    sid = setsid();
    if (sid < 0) {
        g_logger.error("setsid fail, error:%s", strerror(errno));
        return false;
    }

    pid = fork();
    switch(pid) {
        case -1: 
            g_logger.error("2st fork fail, error:%s", strerror(errno));
            return false;
        case 0:
            break;
        default:
            _exit(0); //parent process exit
    }

    status = chdir(cmd_dirpath);
    if (status != 0) {
        g_logger.error("chdir dir=%s fail, error:%s", cmd_dirpath, strerror(errno));
        return false;
    }

    //umask(0);
    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }

    g_logger.set_logdir(g_config.cmd_dirpath);
    g_logger.set_logname("moon.log");

    return true;
}
bool moon_config_t::check_cmd_arg() {
    if (cmd_port <= 0 || cmd_port >= 65535) {
        g_logger.error("port=%d invalid", cmd_port);
        return false;
    }

    if (meta_port <=0 || meta_port >= 65535) {
        g_logger.error("meta port=%d invalid", meta_port);
        return false;
    }
    
    if (meta_ip[0] == '\0') {
        g_logger.error("meta host option empty");
        return false;
    }

    if (cmd_dirpath[0] == '\0') {
        g_logger.error("dir cmd option empty");
        return false;
    }
    int ret = exist_dir(cmd_dirpath);
    if (ret != 0) {
        g_logger.error("dir=%s invalid", cmd_dirpath);
        return false;
    }


    return true;
}

bool moon_config_t::get_local_ip() {
    if (!get_inner_ip(local_ip, sizeof(local_ip))) {
        g_logger.error("get local ip fail");
        return false;
    }
    
    if (!ip2number(local_ip, cmd_port, &server_id)) {
        g_logger.error("ip=%s invalid", local_ip);
        return false;
    }

    g_logger.notice("local_ip=%s, port=%d, serverid=%u", local_ip, cmd_port, server_id);
    return true;
}

bool moon_config_t::init_config() {
    if (!get_local_ip()) {
        g_logger.error("get local ip fail");
        return false;
    }

    if (!reg_server()) {
        g_logger.error("reg server fail");
        return false;
    }
    return true;
}

moon_config_t g_config;
} // namespace moon



/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
