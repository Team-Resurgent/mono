/**
 * \file
 */

#include <config.h>

#ifdef HOST_WIN32
/* For select */
#include <winsock2.h>
#endif

#include "mono-poll.h"
#include <errno.h>
#include <mono/utils/mono-errno.h>

#ifdef DISABLE_SOCKETS
#include <glib.h>

int
mono_poll_can_add (mono_pollfd *ufds, unsigned int nfds, int fd)
{
	return 1;
}

int
mono_poll (mono_pollfd *ufds, unsigned int nfds, int timeout)
{
	g_assert_not_reached ();
	return -1;
}

#else

#if defined(HAVE_POLL) && !defined(__APPLE__)

int
mono_poll_can_add (mono_pollfd *ufds, unsigned int nfds, int fd)
{
	return 1;
}

int
mono_poll (mono_pollfd *ufds, unsigned int nfds, int timeout)
{
	return poll (ufds, nfds, timeout);
}
#else

int
mono_poll_can_add (mono_pollfd *ufds, unsigned int nfds, int fd)
{
	if (fd < 0)
		return 1;
#ifdef HOST_WIN32
	return (nfds < FD_SETSIZE);
#else
	return (fd < FD_SETSIZE);
#endif
}

int
mono_poll (mono_pollfd *ufds, unsigned int nfds, int timeout)
{
	struct timeval tv, *tvptr;
	int i, fd, events, affected, count;
	fd_set rfds, wfds, efds;
	int nexc = 0;
	int maxfd = 0;

	if (timeout < 0) {
		tvptr = NULL;
	} else {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		tvptr = &tv;
	}

	FD_ZERO (&rfds);
	FD_ZERO (&wfds);
	FD_ZERO (&efds);

	for (i = 0; i < nfds; i++) {
		ufds [i].revents = 0;
		fd = ufds [i].fd;
#ifdef RXDK_XBOX_SELECT
		/* Xbox SOCKET values are kernel handles and often have the high bit set.
		 * Only -1 (INVALID_SOCKET) means "skip this slot". */
		if (fd == -1)
			continue;
#else
		if (fd < 0)
			continue;
#endif

#ifdef HOST_WIN32
		if (nexc >= FD_SETSIZE) {
			ufds [i].revents = MONO_POLLNVAL;
			return 1;
		}
#else
		if (fd >= FD_SETSIZE) {
			ufds [i].revents = MONO_POLLNVAL;
			return 1;
		}
#endif

		events = ufds [i].events;
		if ((events & MONO_POLLIN) != 0)
			FD_SET (fd, &rfds);

		if ((events & MONO_POLLOUT) != 0)
			FD_SET (fd, &wfds);

		FD_SET (fd, &efds);
		nexc++;
		if (fd > maxfd)
			maxfd = fd;
			
	}

#ifdef RXDK_XBOX_SELECT
	/* Xbox winsock returns WSAEINVAL for a nonzero nfds, and for a non-NULL
	 * fd_set whose fd_count is 0. Pass NULL for the sets this poll does not use. */
	affected = select (0,
		rfds.fd_count ? &rfds : NULL,
		wfds.fd_count ? &wfds : NULL,
		efds.fd_count ? &efds : NULL,
		tvptr);
#else
	affected = select (maxfd + 1, &rfds, &wfds, &efds, tvptr);
#endif
	if (affected == -1) {
#ifdef HOST_WIN32
		int error = WSAGetLastError ();
		switch (error) {
		case WSAEFAULT: mono_set_errno (EFAULT); break;
		case WSAEINVAL: mono_set_errno (EINVAL); break;
		case WSAEINTR: mono_set_errno (EINTR); break;
		/* case WSAEINPROGRESS: mono_set_errno (EINPROGRESS); break; */
		case WSAEINPROGRESS: mono_set_errno (EINTR); break;
		case WSAENOTSOCK: mono_set_errno (EBADF); break;
#ifdef ENOSR
		case WSAENETDOWN: mono_set_errno (ENOSR); break;
#endif
		default: mono_set_errno (0);
		}
#endif

		return -1;
	}

	count = 0;
	for (i = 0; i < nfds && affected > 0; i++) {
		fd = ufds [i].fd;
#ifdef RXDK_XBOX_SELECT
		if (fd == -1)
			continue;
#else
		if (fd < 0)
			continue;
#endif

		events = ufds [i].events;
		if ((events & MONO_POLLIN) != 0 && FD_ISSET (fd, &rfds)) {
			ufds [i].revents |= MONO_POLLIN;
			affected--;
		}

		if ((events & MONO_POLLOUT) != 0 && FD_ISSET (fd, &wfds)) {
			ufds [i].revents |= MONO_POLLOUT;
			affected--;
		}

		if (FD_ISSET (fd, &efds)) {
			ufds [i].revents |= MONO_POLLERR;
			affected--;
		}

		if (ufds [i].revents != 0)
			count++;
	}

	return count;
}

#endif

#endif /* #ifndef DISABLE_SOCKETS */
