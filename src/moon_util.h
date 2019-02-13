/***************************************************************************
 * 
 * Copyright (c) 2014 liudong9183@qq.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file bus_log.h
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/03/09 09:30:18
 * @version $Revision$
 * @brief 定义日志模块
 *  
 **/
#ifndef  __BUS_UTIL_H_
#define  __BUS_UTIL_H_
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdint.h>
#include<stdarg.h>
#include<time.h>
#include<string.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<ifaddrs.h>
#include "moon_log.h"

namespace moon{
bool ip2number(char *local_ip, int port, uint32_t *ret);
int check_ip(const char *ip_str);
int assure_dir(const char *dir_str);
int file_stat(const char *file_str, uint64_t *offset);
int exist_dir(const char *dir_str);

/* disallow copy assign */
void tm2time(char *tm_str, size_t tm_sz, uint32_t ts);
void fatal_error();
bool str2num(const char *str, long &num);
bool copy_file(const char *source, const char *dst);
bool get_ifip(const char *ifname, char *ip, uint32_t len);
bool get_inner_ip(char *ip_str, uint32_t len);
}//namespace bus
#endif  //__BUS_UTIL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
