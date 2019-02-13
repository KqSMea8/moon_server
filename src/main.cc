/***************************************************************************
 * 
 * Copyright (c) 2018 liudong9183@qq.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file main.cc
 * @author liudong(liudong9183@qq.com)
 * @date 2018/11/28 14:37:31
 * @version $Revision$
 * @brief 
 *  
 **/

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include "moon_config.h"
#include "moon_log.h"
#include "moon_server.h"
#include "moon_util.h"
void sig_term_handler(int signo) {
    moon::g_logger.notice("receive sig term...");
    moon::g_server.stop_server();
}
void setup_signal_handler()
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    /* sigterm handler */
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
    act.sa_handler = sig_term_handler;
    sigaction(SIGTERM, &act, NULL);
}
void usage()
{
    std::cerr << "Usage: moonbox -p port -D instance_dir -H meta_host -P meta_port" << std::endl;
    std::cerr << "Examp: moonbox -p 60000 -D /data1/moonbox60000 -H m3055i.mars.grid.sina.com.cn -P 3055" << std::endl;
    std::cerr << "Descp:" << std::endl;
    std::cerr << " -p port, --port" << std::endl;
    std::cerr << "      server listen port" << std::endl;
    std::cerr << " -D dir, --dir" << std::endl;
    std::cerr << "      server dir" << std::endl;
    std::cerr << " -H host, --mhost" << std::endl;
    std::cerr << "      meta server host" << std::endl;
    std::cerr << " -P port, --mport" << std::endl;
    std::cerr << "      meta server port" << std::endl;
}

int main(int argc, char* argv[]) {
    if (!moon::g_config.parse_cmd_arg(argc, argv)) {
        moon::g_logger.error("parse command line fail");
        usage();
        return -1;
    }

    if (!moon::g_config.init_config()) {
        moon::g_logger.error("init config fail");
        return -1;
    }

    if (moon::g_config.daemon()) {
        moon::g_logger.error("daemon fail");
        return -1;
    }
    
    if (!moon::g_server.init_server()) {
        moon::g_logger.error("init server fail");
        return -1;
    }

    moon::g_server.start_server(moon::g_config.cmd_port);

    return 0;
}


/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
