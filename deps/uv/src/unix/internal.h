/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UV_UNIX_INTERNAL_H_
#define UV_UNIX_INTERNAL_H_

#include "uv-common.h"

#include <assert.h>
#include <stdlib.h> /* abort */

#if defined(__STRICT_ANSI__)
# define inline __inline
#endif

#undef HAVE_PORTS_FS

#if __linux__
# include "linux/syscalls.h"
#endif /* __linux__ */

#if defined(__sun)
# include <sys/port.h>
# include <port.h>
# ifdef PORT_SOURCE_FILE
#  define HAVE_PORTS_FS 1
# endif
# define futimes(fd, tv) futimesat(fd, (void*)0, tv)
#endif /* __sun */

#if defined(__APPLE__) && !TARGET_OS_IPHONE
# include <CoreServices/CoreServices.h>
#endif

#define UNREACHABLE()                                                         \
  do {                                                                        \
    assert(0 && "unreachable code");                                          \
    abort();                                                                  \
  }                                                                           \
  while (0)

#define SAVE_ERRNO(block)                                                     \
  do {                                                                        \
    int _saved_errno = errno;                                                 \
    do { block; } while (0);                                                  \
    errno = _saved_errno;                                                     \
  }                                                                           \
  while (0)

#if defined(__linux__)
# define UV__POLLIN   UV__EPOLLIN
# define UV__POLLOUT  UV__EPOLLOUT
# define UV__POLLERR  UV__EPOLLERR
# define UV__POLLHUP  UV__EPOLLHUP
#endif

#if defined(__sun)
# define UV__POLLIN   POLLIN
# define UV__POLLOUT  POLLOUT
# define UV__POLLERR  POLLERR
# define UV__POLLHUP  POLLHUP
#endif

#ifndef UV__POLLIN
# define UV__POLLIN   1
#endif

#ifndef UV__POLLOUT
# define UV__POLLOUT  2
#endif

#ifndef UV__POLLERR
# define UV__POLLERR  4
#endif

#ifndef UV__POLLHUP
# define UV__POLLHUP  8
#endif

/* handle flags */
enum {
  UV_CLOSING          = 0x01,   /* uv_close() called but not finished. */
  UV_CLOSED           = 0x02,   /* close(2) finished. */
  UV_STREAM_READING   = 0x04,   /* uv_read_start() called. */
  UV_STREAM_SHUTTING  = 0x08,   /* uv_shutdown() called but not complete. */
  UV_STREAM_SHUT      = 0x10,   /* Write side closed. */
  UV_STREAM_READABLE  = 0x20,   /* The stream is readable */
  UV_STREAM_WRITABLE  = 0x40,   /* The stream is writable */
  UV_STREAM_BLOCKING  = 0x80,   /* Synchronous writes. */
  UV_TCP_NODELAY      = 0x100,  /* Disable Nagle. */
  UV_TCP_KEEPALIVE    = 0x200,  /* Turn on keep-alive. */
  UV_TCP_SINGLE_ACCEPT = 0x400  /* Only accept() when idle. */
};

__attribute__((unused))
static void uv__req_init(uv_loop_t* loop, uv_req_t* req, uv_req_type type) {
  req->type = type;
  uv__req_register(loop, req);
}
#define uv__req_init(loop, req, type) \
  uv__req_init((loop), (uv_req_t*)(req), (type))

/* core */
void uv__handle_init(uv_loop_t* loop, uv_handle_t* handle, uv_handle_type type);
int uv__nonblock(int fd, int set);
int uv__cloexec(int fd, int set);
int uv__socket(int domain, int type, int protocol);
int uv__dup(int fd);
int uv_async_stop(uv_async_t* handle);
void uv__make_close_pending(uv_handle_t* handle);

void uv__io_init(uv__io_t* w, uv__io_cb cb, int fd);
void uv__io_start(uv_loop_t* loop, uv__io_t* w, unsigned int events);
void uv__io_stop(uv_loop_t* loop, uv__io_t* w, unsigned int events);
void uv__io_close(uv_loop_t* loop, uv__io_t* w);
void uv__io_feed(uv_loop_t* loop, uv__io_t* w);
int uv__io_active(const uv__io_t* w, unsigned int events);
void uv__io_poll(uv_loop_t* loop, int timeout); /* in milliseconds or -1 */

/* loop */
int uv__loop_init(uv_loop_t* loop, int default_loop);
void uv__loop_delete(uv_loop_t* loop);
void uv__run_idle(uv_loop_t* loop);
void uv__run_check(uv_loop_t* loop);
void uv__run_prepare(uv_loop_t* loop);

/* error */
uv_err_code uv_translate_sys_error(int sys_errno);
void uv_fatal_error(const int errorno, const char* syscall);

/* stream */
void uv__stream_init(uv_loop_t* loop, uv_stream_t* stream,
    uv_handle_type type);
int uv__stream_open(uv_stream_t*, int fd, int flags);
void uv__stream_destroy(uv_stream_t* stream);
void uv__server_io(uv_loop_t* loop, uv__io_t* w, unsigned int events);
int uv__accept(int sockfd);

/* tcp */
int uv_tcp_listen(uv_tcp_t* tcp, int backlog, uv_connection_cb cb);
int uv__tcp_nodelay(int fd, int on);
int uv__tcp_keepalive(int fd, int on, unsigned int delay);

/* pipe */
int uv_pipe_listen(uv_pipe_t* handle, int backlog, uv_connection_cb cb);

/* timer */
void uv__run_timers(uv_loop_t* loop);
int uv__next_timeout(const uv_loop_t* loop);

/* signal */
void uv__signal_close(uv_signal_t* handle);
void uv__signal_global_once_init(void);
void uv__signal_loop_cleanup();

/* thread pool */
void uv__work_submit(uv_loop_t* loop,
                     struct uv__work *w,
                     void (*work)(struct uv__work *w),
                     void (*done)(struct uv__work *w, int status));
void uv__work_done(uv_async_t* handle, int status);

/* platform specific */
int uv__kqueue_init(uv_loop_t* loop);
int uv__platform_loop_init(uv_loop_t* loop, int default_loop);
void uv__platform_loop_delete(uv_loop_t* loop);

/* various */
void uv__async_close(uv_async_t* handle);
void uv__check_close(uv_check_t* handle);
void uv__fs_event_close(uv_fs_event_t* handle);
void uv__idle_close(uv_idle_t* handle);
void uv__pipe_close(uv_pipe_t* handle);
void uv__poll_close(uv_poll_t* handle);
void uv__prepare_close(uv_prepare_t* handle);
void uv__process_close(uv_process_t* handle);
void uv__stream_close(uv_stream_t* handle);
void uv__tcp_close(uv_tcp_t* handle);
void uv__timer_close(uv_timer_t* handle);
void uv__udp_close(uv_udp_t* handle);
void uv__udp_finish_close(uv_udp_t* handle);

#if defined(__APPLE__)
int uv___stream_fd(uv_stream_t* handle);
#define uv__stream_fd(handle) (uv___stream_fd((uv_stream_t*) (handle)))
#else
#define uv__stream_fd(handle) ((handle)->io_watcher.fd)
#endif /* defined(__APPLE__) */

#ifdef UV__O_NONBLOCK
# define UV__F_NONBLOCK UV__O_NONBLOCK
#else
# define UV__F_NONBLOCK 1
#endif

int uv__make_socketpair(int fds[2], int flags);
int uv__make_pipe(int fds[2], int flags);

#if defined(__APPLE__)
typedef void (*cf_loop_signal_cb)(void*);

void uv__cf_loop_signal(uv_loop_t* loop, cf_loop_signal_cb cb, void* arg);

int uv__fsevents_init(uv_fs_event_t* handle);
int uv__fsevents_close(uv_fs_event_t* handle);

/* OSX < 10.7 has no file events, polyfill them */
#ifndef MAC_OS_X_VERSION_10_7

static const int kFSEventStreamCreateFlagFileEvents = 0x00000010;
static const int kFSEventStreamEventFlagItemCreated = 0x00000100;
static const int kFSEventStreamEventFlagItemRemoved = 0x00000200;
static const int kFSEventStreamEventFlagItemInodeMetaMod = 0x00000400;
static const int kFSEventStreamEventFlagItemRenamed = 0x00000800;
static const int kFSEventStreamEventFlagItemModified = 0x00001000;
static const int kFSEventStreamEventFlagItemFinderInfoMod = 0x00002000;
static const int kFSEventStreamEventFlagItemChangeOwner = 0x00004000;
static const int kFSEventStreamEventFlagItemXattrMod = 0x00008000;
static const int kFSEventStreamEventFlagItemIsFile = 0x00010000;
static const int kFSEventStreamEventFlagItemIsDir = 0x00020000;
static const int kFSEventStreamEventFlagItemIsSymlink = 0x00040000;

#endif /* __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1070 */

#endif /* defined(__APPLE__) */

#endif /* UV_UNIX_INTERNAL_H_ */
