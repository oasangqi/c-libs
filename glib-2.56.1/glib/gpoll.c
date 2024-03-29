/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gpoll.c: poll(2) abstraction
 * Copyright 1998 Owen Taylor
 * Copyright 2008 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"
#include "glibconfig.h"
#include "giochannel.h"
#include <string.h>

/* Uncomment the next line (and the corresponding line in gmain.c) to
 * enable debugging printouts if the environment variable
 * G_MAIN_POLL_DEBUG is set to some value.
 */
/* #define G_MAIN_POLL_DEBUG */

#ifdef _WIN32
/* Always enable debugging printout on Windows, as it is more often
 * needed there...
 */
#define G_MAIN_POLL_DEBUG
#endif

#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_POLL
#  include <poll.h>
#ifdef USE_EPOLL_EV
#include <unistd.h>
#include <fcntl.h>
#endif

/* The poll() emulation on OS/X doesn't handle fds=NULL, nfds=0,
 * so we prefer our own poll emulation.
 */
#if defined(_POLL_EMUL_H_) || defined(BROKEN_POLL)
#undef HAVE_POLL
#endif

#endif /* GLIB_HAVE_SYS_POLL_H */
#ifdef G_OS_UNIX
#include <unistd.h>
#endif /* G_OS_UNIX */
#include <errno.h>

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif /* G_OS_WIN32 */

#include "gpoll.h"

#ifdef G_OS_WIN32
#include "gprintf.h"
#endif

#ifdef G_MAIN_POLL_DEBUG
extern gboolean _g_main_poll_debug;
#endif

#ifdef HAVE_POLL

/**
 * g_poll:
 * @fds: file descriptors to poll
 * @nfds: the number of file descriptors in @fds
 * @timeout: amount of time to wait, in milliseconds, or -1 to wait forever
 *
 * Polls @fds, as with the poll() system call, but portably. (On
 * systems that don't have poll(), it is emulated using select().)
 * This is used internally by #GMainContext, but it can be called
 * directly if you need to block until a file descriptor is ready, but
 * don't want to run the full main loop.
 *
 * Each element of @fds is a #GPollFD describing a single file
 * descriptor to poll. The @fd field indicates the file descriptor,
 * and the @events field indicates the events to poll for. On return,
 * the @revents fields will be filled with the events that actually
 * occurred.
 *
 * On POSIX systems, the file descriptors in @fds can be any sort of
 * file descriptor, but the situation is much more complicated on
 * Windows. If you need to use g_poll() in code that has to run on
 * Windows, the easiest solution is to construct all of your
 * #GPollFDs with g_io_channel_win32_make_pollfd().
 *
 * Returns: the number of entries in @fds whose @revents fields
 * were filled in, or 0 if the operation timed out, or -1 on error or
 * if the call was interrupted.
 *
 * Since: 2.20
 **/
gint
g_poll (GPollFD *fds,
	guint    nfds,
	gint     timeout)
{
  return poll ((struct pollfd *)fds, nfds, timeout);
}

#ifdef USE_EPOLL_EV

#define EVENTS_BASE	64
#define FDS_BASE	1024
gint
g_epoll (GPollCTX *ctx, int timeout)
{
	int i;
	int fd;

	ctx->got = epoll_wait(ctx->backend_fd, ctx->epoll_events, ctx->epoll_eventmax, timeout?timeout:1);
	if (ctx->got <= 0) {
		return ctx->got;
	}
	for (i = 0; i < ctx->got; i++) {
		fd = (uint32_t)ctx->epoll_events[i].data.u64;
		if (fd >= ctx->nfds || ctx->anfds[fd].egen != (uint32_t)(ctx->epoll_events[i].data.u64 >> 32)) {
			continue;
		}
		ctx->anfds[fd].revents = ctx->epoll_events[i].events;
	}
	if (ctx->got == ctx->epoll_eventmax) {   
		ctx->epoll_eventmax += EVENTS_BASE;
		ctx->epoll_events = (struct epoll_event *)realloc (ctx->epoll_events, sizeof(struct epoll_event) * ctx->epoll_eventmax);
	}  
	return ctx->got;
}

void g_epoll_init (GPollCTX *ctx)
{
	do {
		ctx->backend_fd = epoll_create(256);
	} while (ctx->backend_fd < 0);

	fcntl(ctx->backend_fd, F_SETFD, FD_CLOEXEC);

	ctx->epoll_eventmax = EVENTS_BASE;
	ctx->epoll_events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * ctx->epoll_eventmax);
	ctx->nfds = FDS_BASE;
	ctx->anfds = (GPollFD *)calloc(ctx->nfds, sizeof (GPollFD));

	return ;
}

void g_epoll_modify(GPollCTX *ctx, GPollFD *poll_fd, int nev)
{
	struct epoll_event ev;
	int oev;

	if (poll_fd->fd == ctx->nfds) {
		ctx->nfds += FDS_BASE;
		ctx->anfds = (GPollFD *)realloc(ctx->anfds, sizeof(GPollFD) * ctx->nfds);
		memset(ctx->anfds + ctx->nfds - FDS_BASE, 0, sizeof(GPollFD) * FDS_BASE);
	}

	if (!nev) {
		ctx->anfds[poll_fd->fd].events = 0;
		ctx->anfds[poll_fd->fd].revents = 0;
		ctx->anfds[poll_fd->fd].egen = 0;
		epoll_ctl(ctx->backend_fd, EPOLL_CTL_DEL, poll_fd->fd, &ev);
		return;
	}

	oev = ctx->anfds[poll_fd->fd].events;
	if (oev == nev) {
		return;
	}

	ctx->anfds[poll_fd->fd].events = nev;
	ctx->anfds[poll_fd->fd].egen = 0;

	ev.data.u64 = (uint64_t)(uint32_t)poll_fd->fd
		| ((uint64_t)(uint32_t)++ctx->anfds[poll_fd->fd].egen << 32);
	ev.events   = (nev & G_IO_IN ? EPOLLIN: 0)
		| (nev & G_IO_OUT ? EPOLLOUT: 0);
	if (oev) {
		if (!epoll_ctl(ctx->backend_fd, EPOLL_CTL_MOD, poll_fd->fd, &ev)) {
			return;
		}
		if (errno == ENOENT) {   
			if (!epoll_ctl(ctx->backend_fd, EPOLL_CTL_ADD, poll_fd->fd, &ev)) {
				return;
			}
		}
		return;
	}
	if (!epoll_ctl(ctx->backend_fd, EPOLL_CTL_ADD, poll_fd->fd, &ev)) {
		return;
	}
	if (errno == EEXIST) {   
		if (!epoll_ctl(ctx->backend_fd, EPOLL_CTL_MOD, poll_fd->fd, &ev)) {
			return;
		}
	}
	return;
}
#endif


#else	/* !HAVE_POLL */

#ifdef G_OS_WIN32

static int
poll_rest (GPollFD *msg_fd,
           HANDLE  *handles,
           GPollFD *handle_to_fd[],
           gint     nhandles,
           gint     timeout)
{
  DWORD ready;
  GPollFD *f;
  int recursed_result;

  if (msg_fd != NULL)
    {
      /* Wait for either messages or handles
       * -> Use MsgWaitForMultipleObjectsEx
       */
      if (_g_main_poll_debug)
	g_print ("  MsgWaitForMultipleObjectsEx(%d, %d)\n", nhandles, timeout);

      ready = MsgWaitForMultipleObjectsEx (nhandles, handles, timeout,
					   QS_ALLINPUT, MWMO_ALERTABLE);

      if (ready == WAIT_FAILED)
	{
	  gchar *emsg = g_win32_error_message (GetLastError ());
	  g_warning ("MsgWaitForMultipleObjectsEx failed: %s", emsg);
	  g_free (emsg);
	}
    }
  else if (nhandles == 0)
    {
      /* No handles to wait for, just the timeout */
      if (timeout == INFINITE)
	ready = WAIT_FAILED;
      else
        {
          /* Wait for the current process to die, more efficient than SleepEx(). */
          WaitForSingleObjectEx (GetCurrentProcess (), timeout, TRUE);
          ready = WAIT_TIMEOUT;
        }
    }
  else
    {
      /* Wait for just handles
       * -> Use WaitForMultipleObjectsEx
       */
      if (_g_main_poll_debug)
	g_print ("  WaitForMultipleObjectsEx(%d, %d)\n", nhandles, timeout);

      ready = WaitForMultipleObjectsEx (nhandles, handles, FALSE, timeout, TRUE);
      if (ready == WAIT_FAILED)
	{
	  gchar *emsg = g_win32_error_message (GetLastError ());
	  g_warning ("WaitForMultipleObjectsEx failed: %s", emsg);
	  g_free (emsg);
	}
    }

  if (_g_main_poll_debug)
    g_print ("  wait returns %ld%s\n",
	     ready,
	     (ready == WAIT_FAILED ? " (WAIT_FAILED)" :
	      (ready == WAIT_TIMEOUT ? " (WAIT_TIMEOUT)" :
	       (msg_fd != NULL && ready == WAIT_OBJECT_0 + nhandles ? " (msg)" : ""))));

  if (ready == WAIT_FAILED)
    return -1;
  else if (ready == WAIT_TIMEOUT ||
	   ready == WAIT_IO_COMPLETION)
    return 0;
  else if (msg_fd != NULL && ready == WAIT_OBJECT_0 + nhandles)
    {
      msg_fd->revents |= G_IO_IN;

      /* If we have a timeout, or no handles to poll, be satisfied
       * with just noticing we have messages waiting.
       */
      if (timeout != 0 || nhandles == 0)
	return 1;

      /* If no timeout and handles to poll, recurse to poll them,
       * too.
       */
      recursed_result = poll_rest (NULL, handles, handle_to_fd, nhandles, 0);
      return (recursed_result == -1) ? -1 : 1 + recursed_result;
    }
  else if (ready >= WAIT_OBJECT_0 && ready < WAIT_OBJECT_0 + nhandles)
    {
      f = handle_to_fd[ready - WAIT_OBJECT_0];
      f->revents = f->events;
      if (_g_main_poll_debug)
        g_print ("  got event %p\n", (HANDLE) f->fd);

      /* If no timeout and polling several handles, recurse to poll
       * the rest of them.
       */
      if (timeout == 0 && nhandles > 1)
	{
	  /* Poll the handles with index > ready */
          HANDLE  *shorter_handles;
          GPollFD **shorter_handle_to_fd;
          gint     shorter_nhandles;

          shorter_handles = &handles[ready - WAIT_OBJECT_0 + 1];
          shorter_handle_to_fd = &handle_to_fd[ready - WAIT_OBJECT_0 + 1];
          shorter_nhandles = nhandles - (ready - WAIT_OBJECT_0 + 1);

	  recursed_result = poll_rest (NULL, shorter_handles, shorter_handle_to_fd, shorter_nhandles, 0);
	  return (recursed_result == -1) ? -1 : 1 + recursed_result;
	}
      return 1;
    }

  return 0;
}

gint
g_poll (GPollFD *fds,
	guint    nfds,
	gint     timeout)
{
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  GPollFD *handle_to_fd[MAXIMUM_WAIT_OBJECTS];
  GPollFD *msg_fd = NULL;
  GPollFD *f;
  gint nhandles = 0;
  int retval;

  if (_g_main_poll_debug)
    g_print ("g_poll: waiting for");

  for (f = fds; f < &fds[nfds]; ++f)
    {
      if (f->fd == G_WIN32_MSG_HANDLE && (f->events & G_IO_IN))
        {
          if (_g_main_poll_debug && msg_fd == NULL)
            g_print (" MSG");
          msg_fd = f;
        }
      else if (f->fd > 0)
        {
          if (nhandles == MAXIMUM_WAIT_OBJECTS)
            {
              g_warning ("Too many handles to wait for!\n");
              break;
            }
          else
            {
              if (_g_main_poll_debug)
                g_print (" %p", (HANDLE) f->fd);
              handle_to_fd[nhandles] = f;
              handles[nhandles++] = (HANDLE) f->fd;
            }
        }
      f->revents = 0;
    }

  if (_g_main_poll_debug)
    g_print ("\n");

  if (timeout == -1)
    timeout = INFINITE;

  /* Polling for several things? */
  if (nhandles > 1 || (nhandles > 0 && msg_fd != NULL))
    {
      /* First check if one or several of them are immediately
       * available
       */
      retval = poll_rest (msg_fd, handles, handle_to_fd, nhandles, 0);

      /* If not, and we have a significant timeout, poll again with
       * timeout then. Note that this will return indication for only
       * one event, or only for messages.
       */
      if (retval == 0 && (timeout == INFINITE || timeout > 0))
	retval = poll_rest (msg_fd, handles, handle_to_fd, nhandles, timeout);
    }
  else
    {
      /* Just polling for one thing, so no need to check first if
       * available immediately
       */
      retval = poll_rest (msg_fd, handles, handle_to_fd, nhandles, timeout);
    }

  if (retval == -1)
    for (f = fds; f < &fds[nfds]; ++f)
      f->revents = 0;

  return retval;
}

#else  /* !G_OS_WIN32 */

/* The following implementation of poll() comes from the GNU C Library.
 * Copyright (C) 1994, 1996, 1997 Free Software Foundation, Inc.
 */

#include <string.h> /* for bzero on BSD systems */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#ifndef NO_FD_SET
#  define SELECT_MASK fd_set
#else /* !NO_FD_SET */
#  ifndef _AIX
typedef long fd_mask;
#  endif /* _AIX */
#  ifdef _IBMR2
#    define SELECT_MASK void
#  else /* !_IBMR2 */
#    define SELECT_MASK int
#  endif /* !_IBMR2 */
#endif /* !NO_FD_SET */

gint
g_poll (GPollFD *fds,
	guint    nfds,
	gint     timeout)
{
  struct timeval tv;
  SELECT_MASK rset, wset, xset;
  GPollFD *f;
  int ready;
  int maxfd = 0;

  FD_ZERO (&rset);
  FD_ZERO (&wset);
  FD_ZERO (&xset);

  for (f = fds; f < &fds[nfds]; ++f)
    if (f->fd >= 0)
      {
	if (f->events & G_IO_IN)
	  FD_SET (f->fd, &rset);
	if (f->events & G_IO_OUT)
	  FD_SET (f->fd, &wset);
	if (f->events & G_IO_PRI)
	  FD_SET (f->fd, &xset);
	if (f->fd > maxfd && (f->events & (G_IO_IN|G_IO_OUT|G_IO_PRI)))
	  maxfd = f->fd;
      }

  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;

  ready = select (maxfd + 1, &rset, &wset, &xset,
		  timeout == -1 ? NULL : &tv);
  if (ready > 0)
    for (f = fds; f < &fds[nfds]; ++f)
      {
	f->revents = 0;
	if (f->fd >= 0)
	  {
	    if (FD_ISSET (f->fd, &rset))
	      f->revents |= G_IO_IN;
	    if (FD_ISSET (f->fd, &wset))
	      f->revents |= G_IO_OUT;
	    if (FD_ISSET (f->fd, &xset))
	      f->revents |= G_IO_PRI;
	  }
      }

  return ready;
}

#endif /* !G_OS_WIN32 */

#endif	/* !HAVE_POLL */
