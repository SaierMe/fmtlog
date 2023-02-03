﻿/*
MIT License

Copyright (c) 2021 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
//#define FMTLOG_HEADER_ONLY
#include "fmt/format.h"
#if (defined(_WIN32) && !defined(FMTLOG_UNICODE_STRING))
#define FMTLOG_UNICODE_STRING 1
#endif
#ifndef FMTLOG_UNICODE_STRING
#define FMTLOG_UNICODE_STRING 0
#endif
#if FMTLOG_UNICODE_STRING == 1
  #include "fmt/xchar.h"
  #define FMTLOG_CHAR wchar_t
  #define FMTLOG_DWORD uint32_t
  #ifndef FMTLOG_T
  #define FMTLOG_T(t) L ## t
  #endif
  #ifndef FMTLOG_tcslen
  #define FMTLOG_tcslen wcslen
  #endif
  #ifndef FMTLOG_tcsftime
  #define FMTLOG_tcsftime wcsftime
  #endif
  #ifndef FMTLOG_tfopen
  #define FMTLOG_tfopen _wfopen
  #endif
#else
  #define FMTLOG_CHAR char
  #define FMTLOG_DWORD uint16_t
  #ifndef FMTLOG_T
  #define FMTLOG_T(t) t
  #endif
  #ifndef FMTLOG_tcslen
  #define FMTLOG_tcslen strlen
  #endif
  #ifndef FMTLOG_tcsftime
  #define FMTLOG_tcsftime strftime
  #endif
  #ifndef FMTLOG_tfopen
  #define FMTLOG_tfopen fopen
  #endif
#endif
#include <type_traits>
#include <vector>
#include <chrono>
#include <atomic>
#include <thread>
#include <memory>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef _WIN32
#define FAST_THREAD_LOCAL thread_local
#else
#define FAST_THREAD_LOCAL __thread
#endif

// define FMTLOG_BLOCK=1 if log statment should be blocked when queue is full, instead of discarding the msg
#ifndef FMTLOG_BLOCK
#define FMTLOG_BLOCK 0
#endif

#define FMTLOG_LEVEL_DBG 0
#define FMTLOG_LEVEL_INF 1
#define FMTLOG_LEVEL_WRN 2
#define FMTLOG_LEVEL_ERR 3
#define FMTLOG_LEVEL_OFF 4

// define FMTLOG_ACTIVE_LEVEL to turn off low log level in compile time
#ifndef FMTLOG_ACTIVE_LEVEL
#define FMTLOG_ACTIVE_LEVEL FMTLOG_LEVEL_INF
#endif

namespace fmtlogdetail {
template<typename Arg>
struct UnrefPtr : std::false_type
{ using type = Arg; };

template<>
struct UnrefPtr<FMTLOG_CHAR*> : std::false_type
{ using type = FMTLOG_CHAR*; };

template<>
struct UnrefPtr<void*> : std::false_type
{ using type = void*; };

template<typename Arg>
struct UnrefPtr<std::shared_ptr<Arg>> : std::true_type
{ using type = Arg; };

template<typename Arg, typename D>
struct UnrefPtr<std::unique_ptr<Arg, D>> : std::true_type
{ using type = Arg; };

template<typename Arg>
struct UnrefPtr<Arg*> : std::true_type
{ using type = Arg; };

}; // namespace fmtlogdetail

template<int __ = 0>
class fmtlogT
{
public:
  enum LogLevel : uint8_t
  {
    DBG = 0,
    INF,
    WRN,
    ERR,
    OFF
  };

  // Preallocate thread queue for current thread
  static void preallocate() noexcept;

  // Set the file for logging
  static bool setLogFile(const FMTLOG_CHAR* filename, bool truncate = false);

  // Set an existing FILE* for logging, if manageFp is false fmtlog will not buffer log internally
  // and will not close the FILE*
  static void setLogFile(FILE* fp, bool manageFp = false);

  static bool setDailyLogFile(const FMTLOG_CHAR* filename, bool truncate = false, int32_t hour = 0, int32_t second = 0, const FMTLOG_CHAR* DateFormat = FMTLOG_T("_%F")) noexcept;

  static void closeDailyLogFile() noexcept;

  // Collect log msgs from all threads and write to log file
  // If forceFlush = true, internal file buffer is flushed
  // User need to call poll() repeatedly if startPollingThread is not used
  static void poll(bool forceFlush = false);

  // Set flush delay in nanosecond
  // If there's msg older than ns in the buffer, flush will be triggered
  static void setFlushDelay(int64_t ns) noexcept;

  // If current msg has level >= flushLogLevel, flush will be triggered
  static void flushOn(LogLevel flushLogLevel) noexcept;

  // If file buffer has more than specified bytes, flush will be triggered
  static void setFlushBufSize(uint32_t bytes) noexcept;

  // callback signature user can register
  // ns: nanosecond timestamp
  // level: logLevel
  // location: full file path with line num, e.g: /home/raomeng/fmtlog/fmtlog.h:45
  // basePos: file base index in the location
  // threadName: thread id or the name user set with setThreadName
  // msg: full log msg with header
  // bodyPos: log body index in the msg
  // logFilePos: log file position of this msg
  typedef void (*LogCBFn)(int64_t ns, LogLevel level, fmt::basic_string_view<FMTLOG_CHAR> location, size_t basePos,
                          fmt::basic_string_view<FMTLOG_CHAR> threadName, fmt::basic_string_view<FMTLOG_CHAR> msg, size_t bodyPos,
                          size_t logFilePos, void* userData);

  // Set a callback function for all log msgs with a mininum log level
  static void setLogCB(LogCBFn cb, LogLevel minCBLogLevel) noexcept;

  typedef void (*LogQFullCBFn)(void* userData);
  static void setLogQFullCB(LogQFullCBFn cb, void* userData) noexcept;

  // Close the log file and subsequent msgs will not be written into the file,
  // but callback function can still be used
  static void closeLogFile() noexcept;

  // Set log header pattern with fmt named arguments
  static void setHeaderPattern(const FMTLOG_CHAR* pattern);

  // Set a name for current thread, it'll be shown in {t} part in header pattern
  static void setThreadName(const FMTLOG_CHAR* name) noexcept;

  // Set current log level, lower level log msgs will be discarded
  static inline void setLogLevel(LogLevel logLevel) noexcept;

  // Get current log level
  static inline LogLevel getLogLevel() noexcept;

  // return true if passed log level is not lower than current log level
  static inline bool checkLogLevel(LogLevel logLevel) noexcept;

  // Run a polling thread in the background with a polling interval in ns
  // Note that user must not call poll() himself when the thread is running
  static void startPollingThread(int64_t pollInterval = 1000000000) noexcept;

  // Stop the polling thread
  static void stopPollingThread() noexcept;

  // https://github.com/MengRao/SPSC_Queue
  class SPSCVarQueueOPT
  {
  public:
    struct MsgHeader
    {
      inline void push(uint32_t sz) { *(volatile uint32_t*)&size = sz + sizeof(MsgHeader); }

      uint32_t size;
      uint32_t logId;
    };
    static constexpr uint32_t BLK_CNT = (1 << 20) / sizeof(MsgHeader);

    MsgHeader* allocMsg(uint32_t size) noexcept;

    MsgHeader* alloc(uint32_t size) {
      size += sizeof(MsgHeader);
      uint32_t blk_sz = (size + sizeof(MsgHeader) - 1) / sizeof(MsgHeader);
      if (blk_sz >= free_write_cnt) {
        uint32_t read_idx_cache = *(volatile uint32_t*)&read_idx;
        if (read_idx_cache <= write_idx) {
          free_write_cnt = BLK_CNT - write_idx;
          if (blk_sz >= free_write_cnt && read_idx_cache != 0) { // wrap around
            blk[0].size = 0;
            blk[write_idx].size = 1;
            write_idx = 0;
            free_write_cnt = read_idx_cache;
          }
        }
        else {
          free_write_cnt = read_idx_cache - write_idx;
        }
        if (free_write_cnt <= blk_sz) {
          return nullptr;
        }
      }
      MsgHeader* ret = &blk[write_idx];
      write_idx += blk_sz;
      free_write_cnt -= blk_sz;
      blk[write_idx].size = 0;
      return ret;
    }

    inline const MsgHeader* front() {
      uint32_t size = blk[read_idx].size;
      if (size == 1) { // wrap around
        read_idx = 0;
        size = blk[0].size;
      }
      if (size == 0) return nullptr;
      return &blk[read_idx];
    }

    inline void pop() {
      uint32_t blk_sz = (blk[read_idx].size + sizeof(MsgHeader) - 1) / sizeof(MsgHeader);
      *(volatile uint32_t*)&read_idx = read_idx + blk_sz;
    }

  private:
    alignas(64) MsgHeader blk[BLK_CNT] = {};
    uint32_t write_idx = 0;
    uint32_t free_write_cnt = BLK_CNT;

    alignas(128) uint32_t read_idx = 0;
  };

  struct ThreadBuffer
  {
    SPSCVarQueueOPT varq;
    bool shouldDeallocate = false;
    FMTLOG_CHAR name[32];
    size_t nameSize;
  };

  // https://github.com/MengRao/tscns
  class TSCNS
  {
  public:
    static const int64_t NsPerSec = 1000000000;

    void init(int64_t init_calibrate_ns = 20000000, int64_t calibrate_interval_ns = 3 * NsPerSec) {
      calibate_interval_ns_ = calibrate_interval_ns;
      int64_t base_tsc, base_ns;
      syncTime(base_tsc, base_ns);
      int64_t expire_ns = base_ns + init_calibrate_ns;
      while (rdsysns() < expire_ns) std::this_thread::yield();
      int64_t delayed_tsc, delayed_ns;
      syncTime(delayed_tsc, delayed_ns);
      double init_ns_per_tsc = (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);
      saveParam(base_tsc, base_ns, base_ns, init_ns_per_tsc);
    }

    void calibrate() {
      if (rdtsc() < next_calibrate_tsc_) return;
      int64_t tsc, ns;
      syncTime(tsc, ns);
      int64_t calulated_ns = tsc2ns(tsc);
      int64_t ns_err = calulated_ns - ns;
      int64_t expected_err_at_next_calibration =
        ns_err + (ns_err - base_ns_err_) * calibate_interval_ns_ / (ns - base_ns_ + base_ns_err_);
      double new_ns_per_tsc =
        ns_per_tsc_ * (1.0 - (double)expected_err_at_next_calibration / calibate_interval_ns_);
      saveParam(tsc, calulated_ns, ns, new_ns_per_tsc);
    }

    static inline int64_t rdtsc() {
#ifdef _MSC_VER
      return __rdtsc();
#elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
      return __builtin_ia32_rdtsc();
#else
      return rdsysns();
#endif
    }

    inline int64_t tsc2ns(int64_t tsc) const {
      while (true) {
        uint32_t before_seq = param_seq_.load(std::memory_order_acquire) & ~1;
        std::atomic_signal_fence(std::memory_order_acq_rel);
        int64_t ns = base_ns_ + (int64_t)((tsc - base_tsc_) * ns_per_tsc_);
        std::atomic_signal_fence(std::memory_order_acq_rel);
        uint32_t after_seq = param_seq_.load(std::memory_order_acquire);
        if (before_seq == after_seq) return ns;
      }
    }

    inline int64_t rdns() const { return tsc2ns(rdtsc()); }

    static inline int64_t rdsysns() {
      using namespace std::chrono;
      return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    }

    double getTscGhz() const { return 1.0 / ns_per_tsc_; }

    // Linux kernel sync time by finding the first trial with tsc diff < 50000
    // We try several times and return the one with the mininum tsc diff.
    // Note that MSVC has a 100ns resolution clock, so we need to combine those ns with the same
    // value, and drop the first and the last value as they may not scan a full 100ns range
    static void syncTime(int64_t& tsc_out, int64_t& ns_out) {
#ifdef _MSC_VER
      const int N = 15;
#else
      const int N = 3;
#endif
      int64_t tsc[N + 1];
      int64_t ns[N + 1];

      tsc[0] = rdtsc();
      for (int i = 1; i <= N; i++) {
        ns[i] = rdsysns();
        tsc[i] = rdtsc();
      }

#ifdef _MSC_VER
      int j = 1;
      for (int i = 2; i <= N; i++) {
        if (ns[i] == ns[i - 1]) continue;
        tsc[j - 1] = tsc[i - 1];
        ns[j++] = ns[i];
      }
      j--;
#else
      int j = N + 1;
#endif

      int best = 1;
      for (int i = 2; i < j; i++) {
        if (tsc[i] - tsc[i - 1] < tsc[best] - tsc[best - 1]) best = i;
      }
      tsc_out = (tsc[best] + tsc[best - 1]) >> 1;
      ns_out = ns[best];
    }

    void saveParam(int64_t base_tsc, int64_t base_ns, int64_t sys_ns, double new_ns_per_tsc) {
      base_ns_err_ = base_ns - sys_ns;
      next_calibrate_tsc_ = base_tsc + (int64_t)((calibate_interval_ns_ - 1000) / new_ns_per_tsc);
      uint32_t seq = param_seq_.load(std::memory_order_relaxed);
      param_seq_.store(++seq, std::memory_order_release);
      std::atomic_signal_fence(std::memory_order_acq_rel);
      base_tsc_ = base_tsc;
      base_ns_ = base_ns;
      ns_per_tsc_ = new_ns_per_tsc;
      std::atomic_signal_fence(std::memory_order_acq_rel);
      param_seq_.store(++seq, std::memory_order_release);
    }

    alignas(64) std::atomic<uint32_t> param_seq_ = 0;
    double ns_per_tsc_;
    int64_t base_tsc_;
    int64_t base_ns_;
    int64_t calibate_interval_ns_;
    int64_t base_ns_err_;
    int64_t next_calibrate_tsc_;
  };

  void init() {
    tscns.init();
    currentLogLevel = INF;
  }

  using Context = fmt::buffer_context<FMTLOG_CHAR>;
  using MemoryBuffer = fmt::basic_memory_buffer<FMTLOG_CHAR, 10000>;
  typedef const FMTLOG_CHAR* (*FormatToFn)(fmt::basic_string_view<FMTLOG_CHAR> format, const FMTLOG_CHAR* data, MemoryBuffer& out,
                                           int& argIdx, std::vector<fmt::basic_format_arg<Context>>& args);

  static void registerLogInfo(uint32_t& logId, FormatToFn fn, const FMTLOG_CHAR* location, LogLevel level,
                              fmt::basic_string_view<FMTLOG_CHAR> fmtString) noexcept;

  static void vformat_to(MemoryBuffer& out, fmt::basic_string_view<FMTLOG_CHAR> fmt, fmt::basic_format_args<Context> args);

  static size_t formatted_size(fmt::basic_string_view<FMTLOG_CHAR> fmt, fmt::basic_format_args<Context> args);

  static void vformat_to(FMTLOG_CHAR* out, fmt::basic_string_view<FMTLOG_CHAR> fmt, fmt::basic_format_args<Context> args);

  static typename SPSCVarQueueOPT::MsgHeader* allocMsg(uint32_t size, bool logQFullCB) noexcept;

  TSCNS tscns;

  volatile LogLevel currentLogLevel;
  static FAST_THREAD_LOCAL ThreadBuffer* threadBuffer;

  template<typename Arg>
  static inline constexpr bool isNamedArg() {
    return fmt::detail::is_named_arg<fmt::remove_cvref_t<Arg>>::value;
  }

  template<typename Arg>
  struct unNamedType
  { using type = Arg; };

  template<typename Arg>
  struct unNamedType<fmt::detail::named_arg<FMTLOG_CHAR, Arg>>
  { using type = Arg; };

#if FMT_USE_NONTYPE_TEMPLATE_ARGS
  template<typename Arg, size_t N, fmt::detail_exported::fixed_string<FMTLOG_CHAR, N> Str>
  struct unNamedType<fmt::detail::statically_named_arg<Arg, FMTLOG_CHAR, N, Str>>
  { using type = Arg; };
#endif

  template<typename Arg>
  static inline constexpr bool isCstring() {
    return fmt::detail::mapped_type_constant<Arg, Context>::value ==
           fmt::detail::type::cstring_type;
  }

  template<typename Arg>
  static inline constexpr bool isString() {
    return fmt::detail::mapped_type_constant<Arg, Context>::value == fmt::detail::type::string_type;
  }

  template<typename Arg>
  static inline constexpr bool needCallDtor() {
    using ArgType = fmt::remove_cvref_t<Arg>;
    if constexpr (isNamedArg<Arg>()) {
      return needCallDtor<typename unNamedType<ArgType>::type>();
    }
    if constexpr (isString<Arg>()) return false;
    return !std::is_trivially_destructible<ArgType>::value;
  }

  template<size_t CstringIdx>
  static inline constexpr size_t getArgSizes(size_t* cstringSize) {
    return 0;
  }

  template<size_t CstringIdx, typename Arg, typename... Args>
  static inline constexpr size_t getArgSizes(size_t* cstringSize, const Arg& arg,
                                             const Args&... args) {
    if constexpr (isNamedArg<Arg>()) {
      return getArgSizes<CstringIdx>(cstringSize, arg.value, args...);
    }
    else if constexpr (isCstring<Arg>()) {
      size_t len = FMTLOG_tcslen(arg) + 1;
      cstringSize[CstringIdx] = len;
      return len + getArgSizes<CstringIdx + 1>(cstringSize, args...);
    }
    else if constexpr (isString<Arg>()) {
      size_t len = arg.size() + 1;
      return len + getArgSizes<CstringIdx>(cstringSize, args...);
    }
    else {
      return sizeof(Arg) + getArgSizes<CstringIdx>(cstringSize, args...);
    }
  }

  template<size_t CstringIdx>
  static inline constexpr FMTLOG_CHAR* encodeArgs(size_t* cstringSize, FMTLOG_CHAR* out) {
    return out;
  }

  template<size_t CstringIdx, typename Arg, typename... Args>
  static inline constexpr FMTLOG_CHAR* encodeArgs(size_t* cstringSize, FMTLOG_CHAR* out, Arg&& arg,
                                                  Args&&... args) {
    if constexpr (isNamedArg<Arg>()) {
      return encodeArgs<CstringIdx>(cstringSize, out, arg.value, std::forward<Args>(args)...);
    }
    else if constexpr (isCstring<Arg>()) {
      memcpy(out, arg, cstringSize[CstringIdx]);
      return encodeArgs<CstringIdx + 1>(cstringSize, out + cstringSize[CstringIdx],
                                        std::forward<Args>(args)...);
    }
    else if constexpr (isString<Arg>()) {
      size_t len = arg.size();
      memcpy(out, arg.data(), len);
      out[len] = 0;
      return encodeArgs<CstringIdx>(cstringSize, out + len + 1, std::forward<Args>(args)...);
    }
    else {
      // If Arg has alignment >= 16, gcc could emit aligned move instructions(e.g. movdqa) for
      // placement new even if the *out* is misaligned, which would cause segfault. So we use memcpy
      // when possible
      if constexpr (std::is_trivially_copyable_v<fmt::remove_cvref_t<Arg>>) {
        memcpy(out, &arg, sizeof(Arg));
      }
      else {
        new (out) fmt::remove_cvref_t<Arg>(std::forward<Arg>(arg));
      }
      return encodeArgs<CstringIdx>(cstringSize, out + sizeof(Arg), std::forward<Args>(args)...);
    }
  }

  template<size_t Idx, size_t NamedIdx>
  static inline constexpr void storeNamedArgs(fmt::detail::named_arg_info<FMTLOG_CHAR>* named_args_store) {
  }

  template<size_t Idx, size_t NamedIdx, typename Arg, typename... Args>
  static inline constexpr void storeNamedArgs(fmt::detail::named_arg_info<FMTLOG_CHAR>* named_args_store,
                                              const Arg& arg, const Args&... args) {
    if constexpr (isNamedArg<Arg>()) {
      named_args_store[NamedIdx] = {arg.name, Idx};
      storeNamedArgs<Idx + 1, NamedIdx + 1>(named_args_store, args...);
    }
    else {
      storeNamedArgs<Idx + 1, NamedIdx>(named_args_store, args...);
    }
  }

  template<bool ValueOnly, size_t Idx, size_t DestructIdx>
  static inline const FMTLOG_CHAR* decodeArgs(const FMTLOG_CHAR* in, fmt::basic_format_arg<Context>* args,
                                              const FMTLOG_CHAR** destruct_args) {
    return in;
  }

  template<bool ValueOnly, size_t Idx, size_t DestructIdx, typename Arg, typename... Args>
  static inline const FMTLOG_CHAR* decodeArgs(const FMTLOG_CHAR* in, fmt::basic_format_arg<Context>* args,
                                              const FMTLOG_CHAR** destruct_args) {
    using namespace fmtlogdetail;
    using ArgType = fmt::remove_cvref_t<Arg>;
    if constexpr (isNamedArg<ArgType>()) {
      return decodeArgs<ValueOnly, Idx, DestructIdx, typename unNamedType<ArgType>::type, Args...>(
        in, args, destruct_args);
    }
    else if constexpr (isCstring<Arg>() || isString<Arg>()) {
      size_t size = FMTLOG_tcslen(in);
      fmt::basic_string_view<FMTLOG_CHAR> v(in, size);
      if constexpr (ValueOnly) {
        fmt::detail::value<Context>& value_ = *(fmt::detail::value<Context>*)(args + Idx);
        value_ = fmt::detail::arg_mapper<Context>().map(v);
      }
      else {
        args[Idx] = fmt::detail::make_arg<Context>(v);
      }
      return decodeArgs<ValueOnly, Idx + 1, DestructIdx, Args...>(in + size + 1, args,
                                                                  destruct_args);
    }
    else {
      if constexpr (ValueOnly) {
        fmt::detail::value<Context>& value_ = *(fmt::detail::value<Context>*)(args + Idx);
        if constexpr (UnrefPtr<ArgType>::value) {
          value_ = fmt::detail::arg_mapper<Context>().map(**(ArgType*)in);
        }
        else {
          value_ = fmt::detail::arg_mapper<Context>().map(*(ArgType*)in);
        }
      }
      else {
        if constexpr (UnrefPtr<ArgType>::value) {
          args[Idx] = fmt::detail::make_arg<Context>(**(ArgType*)in);
        }
        else {
          args[Idx] = fmt::detail::make_arg<Context>(*(ArgType*)in);
        }
      }

      if constexpr (needCallDtor<Arg>()) {
        destruct_args[DestructIdx] = in;
        return decodeArgs<ValueOnly, Idx + 1, DestructIdx + 1, Args...>(in + sizeof(ArgType), args,
                                                                        destruct_args);
      }
      else {
        return decodeArgs<ValueOnly, Idx + 1, DestructIdx, Args...>(in + sizeof(ArgType), args,
                                                                    destruct_args);
      }
    }
  }

  template<size_t DestructIdx>
  static inline void destructArgs(const FMTLOG_CHAR** destruct_args) {}

  template<size_t DestructIdx, typename Arg, typename... Args>
  static inline void destructArgs(const FMTLOG_CHAR** destruct_args) {
    using ArgType = fmt::remove_cvref_t<Arg>;
    if constexpr (isNamedArg<ArgType>()) {
      destructArgs<DestructIdx, typename unNamedType<ArgType>::type, Args...>(destruct_args);
    }
    else if constexpr (needCallDtor<Arg>()) {
      ((ArgType*)destruct_args[DestructIdx])->~ArgType();
      destructArgs<DestructIdx + 1, Args...>(destruct_args);
    }
    else {
      destructArgs<DestructIdx, Args...>(destruct_args);
    }
  }

  template<typename... Args>
  static const FMTLOG_CHAR* formatTo(fmt::basic_string_view<FMTLOG_CHAR> format, const FMTLOG_CHAR* data, MemoryBuffer& out,
                                     int& argIdx, std::vector<fmt::basic_format_arg<Context>>& args) {
    constexpr size_t num_args = sizeof...(Args);
    constexpr size_t num_dtors = fmt::detail::count<needCallDtor<Args>()...>();
    const FMTLOG_CHAR* dtor_args[(std::max)(num_dtors, (size_t)1)];
    const FMTLOG_CHAR* ret;
    if (argIdx < 0) {
      argIdx = (int)args.size();
      args.resize(argIdx + num_args);
      ret = decodeArgs<false, 0, 0, Args...>(data, args.data() + argIdx, dtor_args);
    }
    else {
      ret = decodeArgs<true, 0, 0, Args...>(data, args.data() + argIdx, dtor_args);
    }
    vformat_to(out, format, fmt::basic_format_args(args.data() + argIdx, num_args));
    destructArgs<0, Args...>(dtor_args);

    return ret;
  }

  template<bool Reorder, typename... Args>
  static fmt::basic_string_view<FMTLOG_CHAR> unNameFormat(fmt::basic_string_view<FMTLOG_CHAR> in, uint32_t* reorderIdx,
                                                          const Args&... args) {
    constexpr size_t num_named_args = fmt::detail::count<isNamedArg<Args>()...>();
    if constexpr (num_named_args == 0) {
      return in;
    }
    const FMTLOG_CHAR* begin = in.data();
    const FMTLOG_CHAR* p = begin;
    std::unique_ptr<FMTLOG_CHAR[]> unnamed_str(new FMTLOG_CHAR[in.size() + 1 + num_named_args * 5]);
    fmt::detail::named_arg_info<FMTLOG_CHAR> named_args[(std::max)(num_named_args, (size_t)1)];
    storeNamedArgs<0, 0>(named_args, args...);

    FMTLOG_CHAR* out = (FMTLOG_CHAR*)unnamed_str.get();
    uint8_t arg_idx = 0;
    while (true) {
      auto c = *p++;
      if (!c) {
        size_t copy_size = p - begin - 1;
        memcpy(out, begin, copy_size * sizeof(FMTLOG_CHAR));
        out += copy_size;
        break;
      }
      if (c != '{') continue;
      size_t copy_size = p - begin;
      memcpy(out, begin, copy_size * sizeof(FMTLOG_CHAR));
      out += copy_size;
      begin = p;
      c = *p++;
      if (!c) fmt::detail::throw_format_error("invalid format string");
      if (fmt::detail::is_name_start(c)) {
        while ((fmt::detail::is_name_start(c = *p) || ('0' <= c && c <= '9'))) {
          ++p;
        }
        fmt::basic_string_view<FMTLOG_CHAR> name(begin, p - begin);
        int id = -1;
        for (size_t i = 0; i < num_named_args; ++i) {
          if (named_args[i].name == name) {
            id = named_args[i].id;
            break;
          }
        }
        if (id < 0) fmt::detail::throw_format_error("invalid format string");
        if constexpr (Reorder) {
          reorderIdx[id] = arg_idx++;
        }
        else {
          out = fmt::format_to(out, FMTLOG_T("{}"), id);
        }
      }
      else {
        *out++ = c;
      }
      begin = p;
    }
    const FMTLOG_CHAR* ptr = unnamed_str.release();
    return fmt::basic_string_view<FMTLOG_CHAR>(ptr, out - ptr);
  }

public:
  template<typename... Args>
  inline void log(
    uint32_t& logId, int64_t tsc, const FMTLOG_CHAR* location, LogLevel level,
    fmt::format_string<typename fmtlogdetail::UnrefPtr<fmt::remove_cvref_t<Args>>::type...> format,
    Args&&... args) noexcept {
    if (!logId) {
      auto unnamed_format = unNameFormat<false>(fmt::basic_string_view<FMTLOG_CHAR>(format), nullptr, args...);
      registerLogInfo(logId, formatTo<Args...>, location, level, unnamed_format);
    }
    constexpr size_t num_cstring = fmt::detail::count<isCstring<Args>()...>();
    size_t cstringSizes[(std::max)(num_cstring, (size_t)1)];
    uint32_t alloc_size = 8 + (uint32_t)getArgSizes<0>(cstringSizes, args...) * sizeof(FMTLOG_CHAR);
    bool q_full_cb = true;
    do {
      if (auto header = allocMsg(alloc_size, q_full_cb)) {
        header->logId = logId;
        FMTLOG_CHAR* out = (FMTLOG_CHAR*)(header + 1);
        *(int64_t*)out = tsc;
        out += (8 / sizeof(FMTLOG_CHAR));
        encodeArgs<0>(cstringSizes, out, std::forward<Args>(args)...);
        header->push(alloc_size);
        break;
      }
      q_full_cb = false;
    } while (FMTLOG_BLOCK);
  }

  template<typename... Args>
  inline void logOnce(const FMTLOG_CHAR* location, LogLevel level, fmt::wformat_string<Args...> format,
                      Args&&... args) {
    fmt::basic_string_view<FMTLOG_CHAR> sv(format);
    auto&& fmt_args = fmt::make_wformat_args(args...);
    uint32_t fmt_size = formatted_size(sv, fmt_args);
    uint32_t alloc_size = 8 + 8 + fmt_size * sizeof(FMTLOG_CHAR);
    bool q_full_cb = true;
    do {
      if (auto header = allocMsg(alloc_size, q_full_cb)) {
        header->logId = (uint32_t)level;
        FMTLOG_CHAR* out = (FMTLOG_CHAR*)(header + 1);
        *(int64_t*)out = tscns.rdtsc();
        out += (8 / sizeof(FMTLOG_CHAR));
        *(const FMTLOG_CHAR**)out = location;
        out += (8 / sizeof(FMTLOG_CHAR));
        vformat_to(out, sv, fmt_args);
        header->push(alloc_size);
        break;
      }
      q_full_cb = false;
    } while (FMTLOG_BLOCK);
  }

  inline void logOnceText(const FMTLOG_CHAR* location, LogLevel level, const FMTLOG_CHAR* logText) {
    uint32_t fmt_size = (uint32_t)FMTLOG_tcslen(logText) * sizeof(FMTLOG_CHAR);
    uint32_t alloc_size = 8 + 8 + fmt_size;
    bool q_full_cb = true;
    do {
      if (auto header = allocMsg(alloc_size, q_full_cb)) {
        header->logId = (uint32_t)level;
        FMTLOG_CHAR* out = (FMTLOG_CHAR*)(header + 1);
        *(int64_t*)out = tscns.rdtsc();
        out += (8 / sizeof(FMTLOG_CHAR));
        *(const FMTLOG_CHAR**)out = location;
        out += (8 / sizeof(FMTLOG_CHAR));
        memcpy(out, logText, fmt_size);
        header->push(alloc_size);
        break;
      }
      q_full_cb = false;
    } while (FMTLOG_BLOCK);
  }

};

using fmtlog = fmtlogT<>;

template<int _>
FAST_THREAD_LOCAL typename fmtlogT<_>::ThreadBuffer* fmtlogT<_>::threadBuffer;

template<int __ = 0>
struct fmtlogWrapper
{ static fmtlog impl; };

template<int _>
fmtlog fmtlogWrapper<_>::impl;

template<int _>
inline void fmtlogT<_>::setLogLevel(LogLevel logLevel) noexcept {
  fmtlogWrapper<>::impl.currentLogLevel = logLevel;
}

template<int _>
inline typename fmtlogT<_>::LogLevel fmtlogT<_>::getLogLevel() noexcept {
  return fmtlogWrapper<>::impl.currentLogLevel;
}

template<int _>
inline bool fmtlogT<_>::checkLogLevel(LogLevel logLevel) noexcept {
#ifdef FMTLOG_NO_CHECK_LEVEL
  return true;
#else
  return logLevel >= fmtlogWrapper<>::impl.currentLogLevel;
#endif
}

#define __FMTLOG_S1(x) #x
#define __FMTLOG_S2(x) __FMTLOG_S1(x)
#define __FMTLOG_LOCATION FMTLOG_T(__FILE__ ":" __FMTLOG_S2(__LINE__))
#define __FMTLOG_SOURCE(F, L) FMTLOG_T(##F##":"##L)

#define FMTLOG(level, format, ...)                                                                 \
  do {                                                                                             \
    static uint32_t logId = 0;                                                                     \
    if (!fmtlog::checkLogLevel(level)) break;                                                      \
    fmtlogWrapper<>::impl.log(logId, fmtlogWrapper<>::impl.tscns.rdtsc(), __FMTLOG_LOCATION,       \
                              level, format, ##__VA_ARGS__);                                       \
  } while (0)

#define FMTLOG_LIMIT(min_interval, level, format, ...)                                             \
  do {                                                                                             \
    static uint32_t logId = 0;                                                                     \
    static int64_t limitNs = 0;                                                                    \
    if (!fmtlog::checkLogLevel(level)) break;                                                      \
    int64_t tsc = fmtlogWrapper<>::impl.tscns.rdtsc();                                             \
    int64_t ns = fmtlogWrapper<>::impl.tscns.tsc2ns(tsc);                                          \
    if (ns < limitNs) break;                                                                       \
    limitNs = ns + min_interval;                                                                   \
    fmtlogWrapper<>::impl.log(logId, tsc, __FMTLOG_LOCATION, level, format, ##__VA_ARGS__);        \
  } while (0)

#define FMTLOG_ONCE(level, format, ...)                                                            \
  do {                                                                                             \
    if (!fmtlog::checkLogLevel(level)) break;                                                      \
    fmtlogWrapper<>::impl.logOnce(__FMTLOG_LOCATION, level, format, ##__VA_ARGS__);                \
  } while (0)

#define FMTLOG_LIMIT_LOC(source_location, min_interval, level, format, ...)                        \
  do {                                                                                             \
    static int64_t limitNs = 0;                                                                    \
    if (!fmtlog::checkLogLevel(level)) break;                                                      \
    int64_t tsc = fmtlogWrapper<>::impl.tscns.rdtsc();                                             \
    int64_t ns = fmtlogWrapper<>::impl.tscns.tsc2ns(tsc);                                          \
    if (ns < limitNs) break;                                                                       \
    limitNs = ns + min_interval;                                                                   \
    fmtlogWrapper<>::impl.logOnce(source_location, level, format, ##__VA_ARGS__);                  \
  } while (0)

#define FMTLOG_ONCE_LOC(source_location, level, format, ...)                                       \
  do {                                                                                             \
    if (!fmtlog::checkLogLevel(level)) break;                                                      \
    fmtlogWrapper<>::impl.logOnce(source_location, level, format, ##__VA_ARGS__);                  \
  } while (0)

#define FMTLOG_LIMIT_PLAIN(source_location, min_interval, level, log_content)                      \
  do {                                                                                             \
    static int64_t limitNs = 0;                                                                    \
    if (!fmtlog::checkLogLevel(level)) break;                                                      \
    int64_t tsc = fmtlogWrapper<>::impl.tscns.rdtsc();                                             \
    int64_t ns = fmtlogWrapper<>::impl.tscns.tsc2ns(tsc);                                          \
    if (ns < limitNs) break;                                                                       \
    limitNs = ns + min_interval;                                                                   \
    fmtlogWrapper<>::impl.logOnceText(source_location, level, log_content);                        \
  } while (0)

#define FMTLOG_ONCE_PLAIN(source_location, level, log_content)                                     \
  do {                                                                                             \
    if (!fmtlog::checkLogLevel(level)) break;                                                      \
    fmtlogWrapper<>::impl.logOnceText(source_location, level, log_content);                        \
  } while (0)

#if FMTLOG_ACTIVE_LEVEL <= FMTLOG_LEVEL_DBG
#define logd(format, ...) FMTLOG(fmtlog::DBG, format, ##__VA_ARGS__)
#define logdo(format, ...) FMTLOG_ONCE(fmtlog::DBG, format, ##__VA_ARGS__)
#define logdl(min_interval, format, ...) FMTLOG_LIMIT(min_interval, fmtlog::DBG, format, ##__VA_ARGS__)
#else
#define logd(format, ...) (void)0
#define logdo(format, ...) (void)0
#define logdl(min_interval, format, ...) (void)0
#endif

#if FMTLOG_ACTIVE_LEVEL <= FMTLOG_LEVEL_INF
#define logi(format, ...) FMTLOG(fmtlog::INF, format, ##__VA_ARGS__)
#define logio(format, ...) FMTLOG_ONCE(fmtlog::INF, format, ##__VA_ARGS__)
#define logil(min_interval, format, ...) FMTLOG_LIMIT(min_interval, fmtlog::INF, format, ##__VA_ARGS__)
#else
#define logi(format, ...) (void)0
#define logio(format, ...) (void)0
#define logil(min_interval, format, ...) (void)0
#endif

#if FMTLOG_ACTIVE_LEVEL <= FMTLOG_LEVEL_WRN
#define logw(format, ...) FMTLOG(fmtlog::WRN, format, ##__VA_ARGS__)
#define logwo(format, ...) FMTLOG_ONCE(fmtlog::WRN, format, ##__VA_ARGS__)
#define logwl(min_interval, format, ...) FMTLOG_LIMIT(min_interval, fmtlog::WRN, format, ##__VA_ARGS__)
#else
#define logw(format, ...) (void)0
#define logwo(format, ...) (void)0
#define logwl(min_interval, format, ...) (void)0
#endif

#if FMTLOG_ACTIVE_LEVEL <= FMTLOG_LEVEL_ERR
#define loge(format, ...) FMTLOG(fmtlog::ERR, format, ##__VA_ARGS__)
#define logeo(format, ...) FMTLOG_ONCE(fmtlog::ERR, format, ##__VA_ARGS__)
#define logel(min_interval, format, ...) FMTLOG_LIMIT(min_interval, fmtlog::ERR, format, ##__VA_ARGS__)
#else
#define loge(format, ...) (void)0
#define logeo(format, ...) (void)0
#define logel(min_interval, format, ...) (void)0
#endif

#ifdef FMTLOG_HEADER_ONLY
#include "fmtlog-inl.h"
#endif
