/***************************************************************************
 * 
 * Copyright (c) 2014 liudong9183@qq.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file bus_util.cc
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/04/29 23:32:46
 * @version $Revision$
 * @brief 
 *  
 **/
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "moon_util.h"

namespace moon {
bool ip2number(char *local_ip, int port, uint32_t *ret) {
    size_t len = strlen(local_ip);
    uint32_t ip_num = 0;
    uint32_t slice = 0, slice_idx = 3;
    for (size_t i = 0; i <= len; ++i) {
        if (local_ip[i] == '.' || local_ip[i] == '\0') {
            //g_logger.notice("slice=%u ip_num=%u slice_idx=%u", slice, ip_num, slice_idx);
            ip_num = ip_num + (slice << (slice_idx * 8));
            slice = 0;
            slice_idx -= 1;
        } else if (local_ip[i] < '0' || local_ip[i] > '9') {
            return false;
        } else {
            slice = slice * 10 + (local_ip[i] - '0');
        }
    }
    *ret = (ip_num << 16) + port;
    return true;
}
int check_ip(const char *ip_str) {
    const char *p = ip_str;
    int slice = 0;
    int slice_idx = 0;

    while (*p != '\0') {
        if (*p == '.') {
            if (slice < 0 || slice > 255) return -1;
            slice = 0;
            slice_idx += 1;
        } else if (*p < '0' || *p > '9') {
            return -1;
        } else {
            slice = slice * 10 + (*p - '0');
        }
        p += 1;
    }

    if (slice_idx != 3)  return -1;
    return 0;
}
void tm2time(char *tm_str, size_t tm_sz, uint32_t ts) {
    time_t ts1 = (time_t)ts;
    struct tm result;
    localtime_r(&ts1, &result);
    snprintf(tm_str, tm_sz, "%d-%d-%d %d:%d:%d", 
            result.tm_year + 1900, result.tm_mon + 1, result.tm_mday, result.tm_hour, result.tm_min, result.tm_sec);
}
bool get_ifip(const char *ifname, char *ip, uint32_t len)
{
     struct ifaddrs *ifaddr, *ifitem;
     struct sockaddr_in *sin = NULL;

     if(getifaddrs(&ifaddr) == -1) 
     {
         g_logger.error("get if addrs fail");
         return false;
     }   
    
     for(ifitem = ifaddr; ifitem != NULL; ifitem = ifitem->ifa_next)
     {   
         if(ifitem->ifa_addr == NULL) continue;
         int family = ifitem->ifa_addr->sa_family; 
    
         if(family != AF_INET)
         {    
            continue;
         }   
         sin = (struct sockaddr_in*)ifitem->ifa_addr;
         if (!strcmp(ifitem->ifa_name, ifname)) {
             snprintf(ip, len, inet_ntoa(sin->sin_addr));
             freeifaddrs(ifaddr);
             return true;
         }   
     }        
     freeifaddrs(ifaddr);
     return false;
}
bool get_inner_ip(char *ip_str, uint32_t len)
{
    if (get_ifip("eth1", ip_str, len))
    {
        return true;
    }
    if (get_ifip("eth0", ip_str, len))
    {
        return true;
    }
    if (get_ifip("eth2", ip_str, len))
    {
        return true;
    }
    if (get_ifip("eth3", ip_str, len))
    {
        return true;
    }
    if (get_ifip("eth4", ip_str, len))
    {
        return true;
    }
    return false;
}
int exist_dir(const char *dir_str) {
    int ret;
    struct stat dir_st;
    ret = stat(dir_str, &dir_st);
    if (ret < 0) {
        if (errno == ENOENT) {
            g_logger.error("path=%s not exist", dir_str);
            return -1;
        }
        g_logger.error("path=%s stat fail, error:%s", dir_str, strerror(errno));
        return -2;
    }
    if (!S_ISDIR(dir_st.st_mode)) {
        g_logger.error("path=%s not directory", dir_str);
        return -2;
    }

    return 0;
}
int assure_dir(const char *dir_str) {
    int ret;
    ret = mkdir(dir_str, 0777);
    if (ret < 0) {
        if (errno == EEXIST) {
            return 0;
        }
        g_logger.error("mkdir=%s fail, error:%s", dir_str, strerror(errno));
        return -1;
    }
    return 0;
}

int file_stat(const char *file_str, uint64_t *offset) {
    int ret;
    struct stat file_st;
    ret = stat(file_str, &file_st);
    if (ret < 0) {
        if (errno == ENOENT) {
            g_logger.error("file=%s not exist", file_str);
            return -1;
        }
        g_logger.error("stat file=%s file, error=%s", file_str, strerror(errno));
        return -2;
    }
    *offset = file_st.st_size;
    return 0;
}

void fatal_error()
{
    g_logger.error("fatal error, exit");
    exit(2);
}
bool str2num(const char *str, long &num)
{
    char *endptr;
    num = strtol(str, &endptr, 10);
    if (*str != '\0' && *endptr == '\0')
        return true;
    return false;
}
bool copy_file(const char* source, const char* dst)
{
    assert(source != NULL && dst != NULL);
    int rfd = open(source, O_RDONLY);
    if (rfd == -1)
    {
        g_logger.error("%s so file open fail,error:%s", source, strerror(errno));
        return false;
    }

    int wfd = open(dst, O_WRONLY|O_CREAT|O_APPEND);
    if (wfd == -1)
    {
        g_logger.error("%s so file open fail,error:%s", dst, strerror(errno));
        return false;
    }
    char buf[1024];
    int rcount;
    int wcount;
    while ((rcount = read(rfd, buf, sizeof(buf))) != 0)
    {
        if (rcount == -1)
        {
            g_logger.error("read %s so file fail,error:%s", source, strerror(errno));
            return false;
        }

        wcount = write(wfd, buf, rcount);
        if (wcount != rcount)
        {
            g_logger.error("write %s so file fail", dst);
            return false;
        }
    }

    close(rfd);
    close(wfd);
    return true;
}
}//namespace bus
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
