/*
** Simple udp flooding utility with some nice features, like source ip
** spoofing, land udp attack mode, and others.
** Copyright (C) 2005-2006 urug.projects@gmail.com
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2 of the License, or (at your
** option) any later version.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
** Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <unistd.h>		/* alarm, getopt */
#include <sys/types.h>		/* socket */
#include <sys/socket.h>		/* inet_ntoa, socket */
#include <arpa/inet.h>		/* inet_ntoa */
#include <stdio.h>		/* printf */
#include <string.h>		/* memset */
#include <stdlib.h>		/* atoi */
#include <time.h>		/* time */
#include <netdb.h>		/* gethostbyname */

#define __FAVOR_BSD
#include <netinet/in_systm.h>	/* ntime -> ip.h (*BSD) */
#include <netinet/in.h>		/* inet_ntoa. */
#include <netinet/ip.h>
#include <netinet/udp.h>

#define TRUE	1
#define FALSE	0
#define VERSION "0.3"


typedef char boolean;
typedef struct
{
    boolean	rsource,	/* random source ip */
                ssource,	/* set source ip */
                rdport,		/* random destination port */
                one,		/* send only one datagram */
                land;		/* land attack */
    int         dport,		/* destination port */
                size;		/* datagram size */
    char	source[16];	/* source ip */
} MODES;


/* Prepare MODES struct */
void	prepare_modes( MODES *junk );

/* Flood */
void	sendto_root( struct sockaddr_in *caddr, socklen_t caddr_len, MODES *modes );
void	sendto_nonroot( struct sockaddr_in *caddr, socklen_t caddr_len, MODES *modes );

/* Count packet check sum */
u_short	in_cksum( u_int16_t *addr, int len );

/* Print help. */
void	banner( char *name );
