/*********************************************\
 * 火山视窗 - 格式日志类                     *
 * 作者: Xelloss                             *
 * 网站: https://piv.ink                     *
 * 邮箱: xelloss@vip.qq.com                  *
 * 版本: 2022/11/23                          *
\*********************************************/

#ifndef _PIV_FMTLOG_H
#define _PIV_FMTLOG_H

// 包含火山视窗基本类,它在火山里的包含顺序比较靠前(全局-110)
#ifndef __VOL_BASE_H__
#include <sys/base/libs/win_base/vol_base.h>
#endif

// 火山的_CT宏跟chrono冲突,需要临时取消定义
#ifdef _CT
#undef _CT
#endif
// 取消Windows的max和min宏
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#ifndef FMTLOG_HEADER_ONLY
#define FMTLOG_HEADER_ONLY
#endif
#ifndef FMTLOG_ACTIVE_LEVEL
#define FMTLOG_ACTIVE_LEVEL FMTLOG_LEVEL_OFF
#endif
#ifndef __FMTLOG_SOURCE
#define __FMTLOG_SOURCE(F, L) _T(##F##":"##L)
#endif

#include "fmtlog.h"

// 重新定义_CT宏
#ifndef _CT
#define _CT(x)  CVolConstString (_T (x))
#endif
// 重新定义Windows的max和min宏
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif // _PIV_FMTLOG_H
