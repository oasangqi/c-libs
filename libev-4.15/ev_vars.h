/*
 * loop member variable declarations
 *
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Marc Alexander Lehmann <libev@schmorp.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

#define VARx(type,name) VAR(name, type name)

VARx(ev_tstamp, now_floor) // 上次更新ev_rt_now的M时间 /* last time we refreshed rt_time */
VARx(ev_tstamp, mn_now)    /* monotonic clock "now" */
VARx(ev_tstamp, rtmn_diff) /* difference realtime - monotonic time */

/* for reverse feeding of events */
VARx(W *, rfeeds) // 按先后顺序存放触发的定时器，再逆序放到pendings[pri]，再逆序调用pendings[pri]的回调
VARx(int, rfeedmax)
VARx(int, rfeedcnt)

VAR (pendings, ANPENDING *pendings [NUMPRI])	// 未决事件优先级数组
VAR (pendingmax, int pendingmax [NUMPRI]) // pendings [i]的容量
VAR (pendingcnt, int pendingcnt [NUMPRI]) // pendings [i]已使用个数
VARx(int, pendingpri) /* highest priority currently pending */
VARx(ev_prepare, pending_w) // 停止事件时，如果事件已在pendings数组中，则将pendings数组的该位置设置为pending_w，回调为pendingcb /* dummy pending watcher */

VARx(ev_tstamp, io_blocktime) 
VARx(ev_tstamp, timeout_blocktime) // 主循环poll一次最小超时时间，默认0，可通过接口设置

VARx(int, backend) // method used to poll events
VARx(int, activecnt) /* total number of active events ("refcount") */
VARx(EV_ATOMIC_T, loop_done)  /* signal by ev_break */

VARx(int, backend_fd)
VARx(ev_tstamp, backend_mintime) // 底层poll一次最小超时时间，由具体使用的poll方法决定 /* assumed typical timer resolution */
VAR (backend_modify, void (*backend_modify)(EV_P_ int fd, int oev, int nev)) // callback to modify event
VAR (backend_poll  , void (*backend_poll)(EV_P_ ev_tstamp timeout)) // callback to poll events

VARx(ANFD *, anfds)
VARx(int, anfdmax)

VAR (evpipe, int evpipe [2])
VARx(ev_io, pipe_w)
VARx(EV_ATOMIC_T, pipe_write_wanted)
VARx(EV_ATOMIC_T, pipe_write_skipped)

#if !defined(_WIN32) || EV_GENWRAP
VARx(pid_t, curpid)
#endif

VARx(char, postfork)  /* true if we need to recreate kernel state after fork */

#if EV_USE_SELECT || EV_GENWRAP
VARx(void *, vec_ri)	// Read Input 
VARx(void *, vec_ro)	// Read Output
VARx(void *, vec_wi)	// Write Input
VARx(void *, vec_wo)	// Write Output
#if defined(_WIN32) || EV_GENWRAP
VARx(void *, vec_eo)
#endif
VARx(int, vec_max)	// 当前节点容量
#endif

#if EV_USE_POLL || EV_GENWRAP
VARx(struct pollfd *, polls)
VARx(int, pollmax)
VARx(int, pollcnt)
VARx(int *, pollidxs) /* maps fds into structure indices */
VARx(int, pollidxmax)
#endif

#if EV_USE_EPOLL || EV_GENWRAP
VARx(struct epoll_event *, epoll_events)
VARx(int, epoll_eventmax)
VARx(int *, epoll_eperms)
VARx(int, epoll_epermcnt)
VARx(int, epoll_epermmax)
#endif

#if EV_USE_KQUEUE || EV_GENWRAP
VARx(pid_t, kqueue_fd_pid)
VARx(struct kevent *, kqueue_changes)
VARx(int, kqueue_changemax)
VARx(int, kqueue_changecnt)
VARx(struct kevent *, kqueue_events)
VARx(int, kqueue_eventmax)
#endif

#if EV_USE_PORT || EV_GENWRAP
VARx(struct port_event *, port_events)
VARx(int, port_eventmax)
#endif

VARx(int *, fdchanges)	// 保存待修改事件的fd
VARx(int, fdchangemax)	// fdchanges数组的容量
VARx(int, fdchangecnt)	// fdchanges数组已使用个数

VARx(ANHE *, timers)	// 保存相对定时器内部四叉树结构的数组
VARx(int, timermax)	// timers数组容量
VARx(int, timercnt)	// timers数组已使用个数

VARx(ANHE *, periodics)
VARx(int, periodicmax)
VARx(int, periodiccnt)

VAR (idles, ev_idle **idles [NUMPRI])
VAR (idlemax, int idlemax [NUMPRI])
VAR (idlecnt, int idlecnt [NUMPRI])
VARx(int, idleall) /* total number */

VARx(struct ev_prepare **, prepares)
VARx(int, preparemax)
VARx(int, preparecnt)

VARx(struct ev_check **, checks)
VARx(int, checkmax)
VARx(int, checkcnt)

VARx(struct ev_fork **, forks)	// fork事件
VARx(int, forkmax)
VARx(int, forkcnt) // 调用了ev_fork_start的次数

VARx(struct ev_cleanup **, cleanups)
VARx(int, cleanupmax)
VARx(int, cleanupcnt)

VARx(EV_ATOMIC_T, async_pending)
VARx(struct ev_async **, asyncs)
VARx(int, asyncmax)
VARx(int, asynccnt)

#if EV_USE_INOTIFY || EV_GENWRAP
VARx(int, fs_fd)
VARx(ev_io, fs_w)
VARx(char, fs_2625) /* whether we are running in linux 2.6.25 or newer */
VAR (fs_hash, ANFS fs_hash [EV_INOTIFY_HASHSIZE])
#endif

VARx(EV_ATOMIC_T, sig_pending)
#if EV_USE_SIGNALFD || EV_GENWRAP
VARx(int, sigfd)
VARx(ev_io, sigfd_w)
VARx(sigset_t, sigfd_set)
#endif

VARx(unsigned int, origflags) /* original loop flags */

VARx(unsigned int, loop_count) /* total number of loop iterations/blocks */
VARx(unsigned int, loop_depth) /* #ev_run enters - #ev_run leaves */

VARx(void *, userdata)
VAR (release_cb, void (*release_cb)(EV_P) EV_THROW)
VAR (acquire_cb, void (*acquire_cb)(EV_P) EV_THROW)
VAR (invoke_cb , void (*invoke_cb) (EV_P))

#undef VARx

