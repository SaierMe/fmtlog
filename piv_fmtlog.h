﻿/*********************************************\
 * 火山视窗 - 格式日志类                     *
 * 作者: Xelloss                             *
 * 网站: https://piv.ink                     *
 * 邮箱: xelloss@vip.qq.com                  *
 * 版本: 2022/11/23                          *
\*********************************************/

#ifndef _PIV_FMTLOG_H
#define _PIV_FMTLOG_H

#if _MSVC_LANG < 201703L
#error 格式日志类(fmtlog)需要 C++17 或更高标准,请使用 Visual Studio 2017 以上版本的编译器
#endif

// 包含火山视窗基本类,它在火山里的包含顺序比较靠前(全局-110)
#ifndef __VOL_BASE_H__
#include <sys/base/libs/win_base/vol_base.h>
#endif

// 火山的_CT宏跟chrono冲突,需要临时取消定义
#ifdef _CT
#undef _CT
#endif

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#ifndef FMTLOG_HEADER_ONLY
#define FMTLOG_HEADER_ONLY
#endif
#ifndef FMTLOG_ACTIVE_LEVEL
#define FMTLOG_ACTIVE_LEVEL FMTLOG_LEVEL_OFF
#endif
#ifndef FMTLOG_UNICODE_STRING
#define FMTLOG_UNICODE_STRING
#endif
#ifndef __FMTLOG_SOURCE
#define __FMTLOG_SOURCE(F, L) _T(##F##":"##L)
#endif

#include "fmtlog.h"

// 重新定义_CT宏
#ifndef _CT
#define _CT(x)  CVolConstString (_T (x))
#endif

#endif // _PIV_FMTLOG_H
