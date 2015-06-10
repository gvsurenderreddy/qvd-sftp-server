/* $OpenBSD: sftp-common.c,v 1.28 2015/01/20 23:14:00 deraadt Exp $ */
/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2001 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* #include "includes.h" */
#include "includes-joni.h"
#include "openbsd-compat/openbsd-compat.h"


#include <sys/param.h>	/* MAX */
#include <sys/types.h>
#include <sys/stat.h>
/*
#include <grp.h>
#include <pwd.h>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#ifdef HAVE_UTIL_H
#include <util.h>
#endif	

#include "xmalloc.h"
#include "ssherr.h"
#include "sshbuf.h"
#include "log.h"

#include "sftp.h"
#include "sftp-common.h"

#ifdef __WIN32__
#include "openbsd-compat/qvd_pw.h"
#include <Windows.h>
#include <winnt.h>
#include <basetyps.h>
#include <Shellapi.h>
#endif

/* Clear contents of attributes structure */
void
attrib_clear(Attrib *a)
{
	a->flags = 0;
	a->size = 0;
	a->uid = 0;
	a->gid = 0;
	a->perm = 0;
	a->atime = 0;
	a->mtime = 0;
}

/* Convert from struct stat to filexfer attribs */
void
stat_to_attrib(const struct stat *st, Attrib *a)
{
	attrib_clear(a);
	a->flags = 0;
	a->flags |= SSH2_FILEXFER_ATTR_SIZE;
	a->size = st->st_size;
	a->flags |= SSH2_FILEXFER_ATTR_UIDGID;
	a->uid = st->st_uid;
	a->gid = st->st_gid;
	a->flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
	a->perm = st->st_mode;
	a->flags |= SSH2_FILEXFER_ATTR_ACMODTIME;
	a->atime = st->st_atime;
	a->mtime = st->st_mtime;
}

#ifdef __WIN32__
static time_t filetime_to_time_t(FILETIME time)
{
	ULARGE_INTEGER tmp;
	tmp.LowPart  = time.dwLowDateTime;
	tmp.HighPart = time.dwHighDateTime;
	return tmp.QuadPart / 10000000ULL - 11644473600ULL;
}

void
find_data_to_attrib(LPWIN32_FIND_DATA file, Attrib *a)
{
	attrib_clear(a);
	a->flags = 0;
	a->flags |= SSH2_FILEXFER_ATTR_SIZE;
	a->size = (file->nFileSizeHigh * (MAXDWORD+1)) + file->nFileSizeLow;
	a->flags |= SSH2_FILEXFER_ATTR_UIDGID;
	a->uid = 0;
	a->gid = 0;
	a->flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
	a->perm = 0644;
	
	if ( file->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
		a->perm = 0755;
		a->perm ^= S_IFDIR;
	} else {
		a->perm ^= S_IFREG;
	}
	
	if ( file->dwFileAttributes & FILE_ATTRIBUTE_READONLY )
		a->perm &= ~0222;
	
	a->flags |= SSH2_FILEXFER_ATTR_ACMODTIME;
	a->atime = filetime_to_time_t( file->ftLastAccessTime );
	a->mtime = filetime_to_time_t( file->ftLastWriteTime );
}

void file_info_to_attrib(LPBY_HANDLE_FILE_INFORMATION file_info, FILE_ID_INFO *fileid_info, Attrib *a)
{
	attrib_clear(a);
	a->flags = 0;
	a->flags |= SSH2_FILEXFER_ATTR_SIZE;
	a->size = (file_info->nFileSizeHigh * (MAXDWORD+1)) + file_info->nFileSizeLow;
	a->flags |= SSH2_FILEXFER_ATTR_UIDGID;
	a->uid = 0;
	a->gid = 0;
	a->flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
	a->perm = 0644;
	
	if ( file_info->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
		a->perm = 0755;
		a->perm ^= S_IFDIR;
	} else {
		a->perm ^= S_IFREG;
	}
	
	if ( file_info->dwFileAttributes & FILE_ATTRIBUTE_READONLY )
		a->perm &= ~0222;
	
	a->flags |= SSH2_FILEXFER_ATTR_ACMODTIME;
	a->atime = filetime_to_time_t( file_info->ftLastAccessTime );
	a->mtime = filetime_to_time_t( file_info->ftLastWriteTime );
	
}


#endif
/* Convert from filexfer attribs to struct stat */
void
attrib_to_stat(const Attrib *a, struct stat *st)
{
	memset(st, 0, sizeof(*st));

	if (a->flags & SSH2_FILEXFER_ATTR_SIZE)
		st->st_size = a->size;
	if (a->flags & SSH2_FILEXFER_ATTR_UIDGID) {
		st->st_uid = a->uid;
		st->st_gid = a->gid;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
		st->st_mode = a->perm;
	if (a->flags & SSH2_FILEXFER_ATTR_ACMODTIME) {
		st->st_atime = a->atime;
		st->st_mtime = a->mtime;
	}
}

/* Decode attributes in buffer */
int
decode_attrib(struct sshbuf *b, Attrib *a)
{
	int r;

	attrib_clear(a);
	if ((r = sshbuf_get_u32(b, &a->flags)) != 0)
		return r;
	if (a->flags & SSH2_FILEXFER_ATTR_SIZE) {
		if ((r = sshbuf_get_u64(b, &a->size)) != 0)
			return r;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_UIDGID) {
		if ((r = sshbuf_get_u32(b, &a->uid)) != 0 ||
		    (r = sshbuf_get_u32(b, &a->gid)) != 0)
			return r;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS) {
		if ((r = sshbuf_get_u32(b, &a->perm)) != 0)
			return r;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_ACMODTIME) {
		if ((r = sshbuf_get_u32(b, &a->atime)) != 0 ||
		    (r = sshbuf_get_u32(b, &a->mtime)) != 0)
			return r;
	}
	/* vendor-specific extensions */
	if (a->flags & SSH2_FILEXFER_ATTR_EXTENDED) {
		char *type;
		u_char *data;
		size_t dlen;
		u_int i, count;

		if ((r = sshbuf_get_u32(b, &count)) != 0)
			fatal("%s: buffer error: %s", __func__, ssh_err(r));
		for (i = 0; i < count; i++) {
			if ((r = sshbuf_get_cstring(b, &type, NULL)) != 0 ||
			    (r = sshbuf_get_string(b, &data, &dlen)) != 0)
				return r;
			debug3("Got file attribute \"%.100s\" len %zu",
			    type, dlen);
			free(type);
			free(data);
		}
	}
	return 0;
}

/* Encode attributes to buffer */
int
encode_attrib(struct sshbuf *b, const Attrib *a)
{
	int r;

	if ((r = sshbuf_put_u32(b, a->flags)) != 0)
		return r;
	if (a->flags & SSH2_FILEXFER_ATTR_SIZE) {
		if ((r = sshbuf_put_u64(b, a->size)) != 0)
			return r;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_UIDGID) {
		if ((r = sshbuf_put_u32(b, a->uid)) != 0 ||
		    (r = sshbuf_put_u32(b, a->gid)) != 0)
			return r;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS) {
		if ((r = sshbuf_put_u32(b, a->perm)) != 0)
			return r;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_ACMODTIME) {
		if ((r = sshbuf_put_u32(b, a->atime)) != 0 ||
		    (r = sshbuf_put_u32(b, a->mtime)) != 0)
			return r;
	}
	return 0;
}

/* Convert from SSH2_FX_ status to text error message */
const char *
fx2txt(int status)
{
	switch (status) {
	case SSH2_FX_OK:
		return("No error");
	case SSH2_FX_EOF:
		return("End of file");
	case SSH2_FX_NO_SUCH_FILE:
		return("No such file or directory");
	case SSH2_FX_PERMISSION_DENIED:
		return("Permission denied");
	case SSH2_FX_FAILURE:
		return("Failure");
	case SSH2_FX_BAD_MESSAGE:
		return("Bad message");
	case SSH2_FX_NO_CONNECTION:
		return("No connection");
	case SSH2_FX_CONNECTION_LOST:
		return("Connection lost");
	case SSH2_FX_OP_UNSUPPORTED:
		return("Operation unsupported");
	default:
		return("Unknown status");
	}
	/* NOTREACHED */
}

/*
 * drwxr-xr-x    5 markus   markus       1024 Jan 13 18:39 .ssh
 */
char *
ls_file(const char *name, const struct stat *st, int remote, int si_units)
{
	int ulen, glen, sz = 0;
	struct tm *ltime = localtime(&st->st_mtime);
	char *user, *group;
	char buf[1024], mode[11+1], tbuf[12+1], ubuf[11+1], gbuf[11+1];
	char sbuf[FMT_SCALED_STRSIZE];
	time_t now;


	strmode(st->st_mode, mode);
	if (!remote) {
		user = user_from_uid(st->st_uid, 0);
	} else {
		snprintf(ubuf, sizeof ubuf, "%u", (u_int)st->st_uid);
		user = ubuf;
	}
	if (!remote) {
		group = group_from_gid(st->st_gid, 0);
	} else {
		snprintf(gbuf, sizeof gbuf, "%u", (u_int)st->st_gid);
		group = gbuf;
	}
	if (ltime != NULL) {
		now = time(NULL);
		if (now - (365*24*60*60)/2 < st->st_mtime &&
		    now >= st->st_mtime)
			sz = strftime(tbuf, sizeof tbuf, "%b %e %H:%M", ltime);
		else
			sz = strftime(tbuf, sizeof tbuf, "%b %e  %Y", ltime);
	}
	if (sz == 0)
		tbuf[0] = '\0';
	ulen = MAX(strlen(user), 8);
	glen = MAX(strlen(group), 8);
	if (si_units) {
		fmt_scaled((long long)st->st_size, sbuf);
		snprintf(buf, sizeof buf, "%s %3u %-*s %-*s %8s %s %s", mode,
		    (u_int)st->st_nlink, ulen, user, glen, group,
		    sbuf, tbuf, name);
	} else {
		snprintf(buf, sizeof buf, "%s %3u %-*s %-*s %8llu %s %s", mode,
		    (u_int)st->st_nlink, ulen, user, glen, group,
		    (unsigned long long)st->st_size, tbuf, name);
	}
	return xstrdup(buf);
}


#ifdef __WIN32__

ULONGLONG time_diff(SYSTEMTIME t1, SYSTEMTIME t2) {
	FILETIME f1, f2;
	ULARGE_INTEGER u1, u2;
	
	if ( SystemTimeToFileTime(&t1, &f1) &&
	     SystemTimeToFileTime(&t2, &f2) ) {
			
		u1.LowPart  = f1.dwLowDateTime;
		u1.HighPart = f1.dwHighDateTime;
		u2.LowPart  = f2.dwLowDateTime;
		u2.HighPart = f2.dwHighDateTime;
		
		return u2.QuadPart - u1.QuadPart;
	}
	
	// Shoudln't really matter
	return 0;
}

char *
win32_ls_file(const char *name, LPWIN32_FIND_DATA file, int remote, int si_units)
{
	int ulen, glen, sz = 0;
	//struct tm *ltime = localtime(&st->st_mtime);
	char *user, *group;
	char buf[1024], mode[11+1], tbuf[12+1], ubuf[11+1], gbuf[11+1];
	char sbuf[FMT_SCALED_STRSIZE];
	SYSTEMTIME now;
	SYSTEMTIME ftime;
	
	time_t mtime = filetime_to_time_t( file->ftLastWriteTime );
	BOOL time_conv_ok = FileTimeToSystemTime( &file->ftLastWriteTime, &ftime);
	struct tm *ltime = localtime( &mtime );
	
    if (!time_conv_ok) {
		error("Failed to convert file time to localtime");
	}
	
	strmode(0644, mode);
	if (!remote) {
		user = user_from_uid(0, 0);
	} else {
		snprintf(ubuf, sizeof ubuf, "%u", 0);
		user = ubuf;
	}
	
	if (!remote) {
		group = group_from_gid(0, 0);
	} else {
		snprintf(gbuf, sizeof gbuf, "%u", 0);
		group = gbuf;
	}
	
	if (time_conv_ok) {
		//now = time(NULL);
		GetSystemTime(&now);
		
		if ( (time_diff(now, ftime) / 10000000ULL) < (365*24*60*60) ) {
		//if (now - (365*24*60*60)/2 < st->st_mtime &&
		  //  now >= st->st_mtime)
			sz = strftime(tbuf, sizeof tbuf, "%b %e %H:%M", ltime);
		} else {
			sz = strftime(tbuf, sizeof tbuf, "%b %e  %Y", ltime);
		}
	}
	if (sz == 0)
		tbuf[0] = '\0';
	ulen = MAX(strlen(user), 8);
	glen = MAX(strlen(group), 8);
	long long size = (file->nFileSizeHigh * (MAXDWORD+1)) + file->nFileSizeLow;
		
	if (si_units) {
		fmt_scaled(size, sbuf);
		snprintf(buf, sizeof buf, "%s %3u %-*s %-*s %8s %s %s", mode,
		    1 /*nlink -- FIXME */, ulen, user, glen, group,
		    sbuf, tbuf, name);
	} else {
		snprintf(buf, sizeof buf, "%s %3u %-*s %-*s %8llu %s %s", mode,
		    1 /*nlink -- FIXME */, ulen, user, glen, group,
		    size, tbuf, name);
	}
	return xstrdup(buf);
}

#endif

