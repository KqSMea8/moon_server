/***************************************************************************
 * 
 * Copyright (c) 2018 liudong9183@qq.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file config.h
 * @author liudong(liudong9183@qq.com)
 * @date 2018/11/28 15:17:30
 * @version $Revision$
 * @brief 
 *  
 **/



#ifndef  __CONFIG_H_
#define  __CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <map>
#include <string>

#include "moon_log.h"
#include "moon_util.h"
namespace moon {

class moon_config_t {
    public:
        moon_config_t();
        ~moon_config_t() {}
        bool parse_cmd_arg(const int argc, char *const *argv);
        bool init_config();
        bool daemon();
    public:
        int cmd_port;
        char cmd_dirpath[1024];

        char meta_ip[32];
        int meta_port;

        char local_ip[32];
        uint32_t server_id;

        bool is_daemon;
    private:
        bool check_cmd_arg();
        bool get_local_ip();
        bool reg_server() {
            return true;
        }
        DISALLOW_COPY_AND_ASSIGN(moon_config_t);
};

extern moon_config_t g_config;
} // namespace moon


#endif  //__CONFIG_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
