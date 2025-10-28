/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#pragma hdrstop
#include "../../idlib/precompiled.h"

/*
================================================================================================
Contains the NetworkSystem implementation specific to Win32.
================================================================================================
*/

#include <iptypes.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>

static WSADATA	winsockdata = {};
static bool	winsockInitialized = false;
static bool usingSocks = false;

//lint -e569	ioctl macros trigger this

// force these libs to be included, so users of idLib don't need to add them to every project
#pragma comment(lib, "iphlpapi.lib" )
#pragma comment(lib, "wsock32.lib" )


/*
================================================================================================

	Network CVars

================================================================================================
*/

static idCVar net_socksServer( "net_socksServer", "", CVAR_ARCHIVE, "" );
static idCVar net_socksPort( "net_socksPort", "1080", CVAR_ARCHIVE | CVAR_INTEGER, "" );
static idCVar net_socksUsername( "net_socksUsername", "", CVAR_ARCHIVE, "" );
static idCVar net_socksPassword( "net_socksPassword", "", CVAR_ARCHIVE, "" );

static idCVar net_ip( "net_ip", "localhost", 0, "local IP address" );

static struct sockaddr_in	socksRelayAddr = {};

static SOCKET	ip_socket = 0;
static SOCKET	socks_socket = 0;
static char		socksBuf[4096] = {};

constexpr size_t MAX_INTERFACES            = 32;
constexpr size_t MAX_UNICAST_PER_INTERFACE = 16;
constexpr size_t MAX_GATEWAY_PER_INTERFACE = 8;
constexpr size_t MAX_DNS_PER_INTERFACE     = 8;
constexpr size_t MAX_INTERFACE_CHARS       = 256;
constexpr size_t MAX_ADAPTER_NAME_LEN      = 256;

typedef struct net_addr_entry_s
{
	SOCKADDR_STORAGE sa;
	uint8            prefix_len;                // for unicast/prefix; 0 for gw/dns
	char             text[INET6_ADDRSTRLEN];    // canonical short text ("x.x.x.x" or "xxxx::")
} net_addr_entry_t;

typedef struct net_interface_s
{
	// Basics
	uint32    IfIndex;       // IPv4 ifindex
	uint32    Ipv6IfIndex;   // IPv6 ifindex (0 if none)
	NET_LUID  Luid;         // stable identity
	uint32    Mtu;

	// State / flags
	uint32   OperStatus;    // IfOperStatus*
	uint32   IfType;        // IF_TYPE_*
	uint32   Flags;         // IP_ADAPTER_* flags (subset from GetAdaptersAddresses)

	// MAC
	uint8    Mac[8];
	size_t   MacLen;        // 0..8

	// Names
	char     AdapterName[MAX_INTERFACE_CHARS];     // GUID-ish
	char     Description[MAX_INTERFACE_CHARS];     // ANSI description (provided by API)
	char     FriendlyName[MAX_INTERFACE_CHARS];    // user-friendly name (UTF-16)

	// Unicast
	size_t             UnicastCount;
	net_addr_entry_t   Unicast[MAX_UNICAST_PER_INTERFACE];

	// Gateways
	size_t             GatewayCount;
	net_addr_entry_t   Gateways[MAX_GATEWAY_PER_INTERFACE];

	// DNS servers
	size_t             DnsCount;
	net_addr_entry_t   Dns[MAX_DNS_PER_INTERFACE];
} net_interface_t;

static size_t			numInterfaces = 0;
static net_interface_t	networkInterfacesArray[MAX_INTERFACES] = {};

/*
================================================================================================

	Free Functions

================================================================================================
*/

/*
========================
NET_ErrorString
========================
*/
static const char *NET_ErrorString() {
	int		code = 0;

	code = WSAGetLastError();
	switch( code ) {
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSAECONNABORTED";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}

/*
========================
Net_Classify
========================
*/
static void Net_Classify(netadr_t* a) {
	if (!a)
	{
		return;
	}
	a->flags = 0;
	a->v6_mcast_scope = 0;

	if (a->type == NA_IPv4 || a->type == NA_BROADCAST || a->type == NA_LOOPBACK) 
	{
		const uint8* p = a->addr.ip4;
		const uint32 v = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];

		if ((v & 0xFF000000u) == 0x7F000000u)
		{
			a->flags |= NETADR_F_LOOPBACK; // 127/8
		}

		if (v == 0xFFFFFFFFu) {                                                     // limited broadcast
			a->flags |= NETADR_F_BROADCAST_V4;
			a->type = NA_BROADCAST; // keep legacy semantic too
		}

		if ((v & 0xF0000000u) == 0xE0000000u)
		{
			a->flags |= NETADR_F_MULTICAST; // 224/4
		}

		if ((v & 0xFFFF0000u) == 0xA9FE0000u)
		{
			a->flags |= NETADR_F_LINKLOCAL; // 169.254/16
		}

		if ((v & 0xFF000000u) == 0x0A000000u)
		{
			a->flags |= NETADR_F_PRIVATE_V4; // 10/8
		}

		if ((v & 0xFFF00000u) == 0xAC100000u)
		{
			a->flags |= NETADR_F_PRIVATE_V4; // 172.16/12
		}

		if ((v & 0xFFFF0000u) == 0xC0A80000u)
		{
			a->flags |= NETADR_F_PRIVATE_V4; // 192.168/16
		}

		if ((v & 0xFFC00000u) == 0x64400000u)
		{
			a->flags |= NETADR_F_CGNAT_V4; // 100.64/10
		}

		if ((v & 0xFFFFFF00u) == 0xC0000200u || /* 192.0.2.0/24 */
			(v & 0xFFFFFF00u) == 0xC6336400u || /* 198.51.100.0/24 */
			(v & 0xFFFFFF00u) == 0xCB007100u)     /* 203.0.113.0/24 */
		{
			a->flags |= NETADR_F_DOC;
		}

		if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0)
		{
			a->flags |= NETADR_F_UNSPECIFIED;
		}

		if (!(a->flags & (NETADR_F_LOOPBACK | NETADR_F_BROADCAST_V4 | NETADR_F_MULTICAST |
			NETADR_F_LINKLOCAL | NETADR_F_PRIVATE_V4 | NETADR_F_CGNAT_V4 | NETADR_F_DOC | NETADR_F_UNSPECIFIED)))
		{
			a->flags |= NETADR_F_GLOBAL;
		}
		return;
	}

	if (a->type == NA_IPv6 || a->type == NA_LOOPBACK) 
	{
		const uint8* p = a->addr.ip6;

		// ::1
		int loop = 1;
		for (size_t i = 0; i < 15; ++i)
		{
			if (p[i]) { loop = 0; break; }
		}
		if (loop && p[15] == 1) { a->flags |= NETADR_F_LOOPBACK; a->type = NA_LOOPBACK; a->v6_scope_id = 0; }

		// ff00::/8
		if (p[0] == 0xFF) {
			a->flags |= NETADR_F_MULTICAST;
			a->v6_mcast_scope = static_cast<uint8>(p[1] & 0x0F);  // scope nibble
		}

		// fe80::/10 (link-local)
		if (p[0] == 0xFE && (p[1] & 0xC0) == 0x80)
		{
			a->flags |= NETADR_F_LINKLOCAL;
		}

		// fec0::/10 (deprecated site-local)
		if (p[0] == 0xFE && (p[1] & 0xC0) == 0xC0)
		{
			a->flags |= NETADR_F_SITELOCAL_V6;
		}

		// fc00::/7 (ULA)
		if ((p[0] & 0xFE) == 0xFC)
		{
			a->flags |= NETADR_F_ULA_V6;
		}

		// 2001:db8::/32 (docs)
		if (p[0] == 0x20 && p[1] == 0x01 && p[2] == 0x0D && p[3] == 0xB8)
		{
			a->flags |= NETADR_F_DOC;
		}

		// :: (unspecified)
		int allzero = 1;
		for (size_t i = 0; i < 16; ++i)
		{
			if (p[i]) { allzero = 0; break; }
		}
		if (allzero)
		{
			a->flags |= NETADR_F_UNSPECIFIED;
		}

		if (!(a->flags & (NETADR_F_LOOPBACK | NETADR_F_MULTICAST | NETADR_F_LINKLOCAL |
			NETADR_F_SITELOCAL_V6 | NETADR_F_ULA_V6 | NETADR_F_DOC | NETADR_F_UNSPECIFIED)))
		{
			a->flags |= NETADR_F_GLOBAL;
		}
	}
}


/*
========================
Net_NetadrToSockadr
========================
*/
static void Net_NetadrToSockadr( const netadr_t *a, SOCKADDR_STORAGE *s ) {
	if (!a || !s)
	{
		return;
	}
	memset(s, 0, sizeof(*s));

	if (a->type == NA_IPv4 || a->type == NA_BROADCAST || a->type == NA_LOOPBACK) {
		struct sockaddr_in* v4 = reinterpret_cast<struct sockaddr_in*>(s);
		v4->sin_family = AF_INET;
		v4->sin_addr = a->addr.in4;
		v4->sin_port = a->port;
		return;
	}
	if (a->type == NA_IPv6 || a->type == NA_LOOPBACK) {
		struct sockaddr_in6* v6 = reinterpret_cast<struct sockaddr_in6*>(s);
		v6->sin6_family = AF_INET6;
		v6->sin6_addr = a->addr.in6;
		v6->sin6_port = a->port;
		v6->sin6_scope_id = a->v6_scope_id; // critical for link-local
		return;
	}
	// NA_BAD: leave zeroed
}

/*
========================
Net_SockadrToNetadr
========================
*/
static void Net_SockadrToNetadr(const SOCKADDR_STORAGE *s, netadr_t *a ) {
	if (!s || !a)
	{
		return;
	}
	memset(a, 0, sizeof(*a));

	const struct sockaddr* sa = reinterpret_cast<const struct sockaddr*>(s);
	if (sa->sa_family == AF_INET) {
		const struct sockaddr_in* v4 = reinterpret_cast<const struct sockaddr_in*>(s);
		a->type = NA_IPv4;
		a->addr.in4 = v4->sin_addr;
		a->port = v4->sin_port;  // network order
		a->v6_scope_id = 0;

		// Optional legacy: tag NA_BROADCAST when 255.255.255.255
		if (v4->sin_addr.s_addr == 0xFFFFFFFFu)
		{
			a->type = NA_BROADCAST;
		}
	}
	else if (sa->sa_family == AF_INET6) {
		const struct sockaddr_in6* v6 = reinterpret_cast<const struct sockaddr_in6*>(s);
		a->type = NA_IPv6;
		a->addr.in6 = v6->sin6_addr;
		a->port = v6->sin6_port;      // network order
		a->v6_scope_id = v6->sin6_scope_id;  // needed for fe80::/10
	}
	else {
		a->type = NA_BAD;
		return;
	}

	Net_Classify(a);
}

/*
========================
Net_ExtractPort
========================
*/
static bool Net_ExtractPort( const char *src, char *buf, const size_t bufSize, uint16 *port ) {
	char *p = nullptr;
	strncpy( buf, src, bufSize );
	p = buf; p += Min( bufSize - 1, idStr::Length( src ) ); *p = '\0';
	p = strchr( buf, ':' );
	if ( !p ) {
		return false;
	}
	*p = '\0';
	*port = numeric_cast<uint16>(strtoul( p+1, nullptr, 10 ));
	if ( errno == ERANGE ) {
		return false;
	}
	return true;
}

/*
========================
Net_ParseHostPort
========================
*/
// Split "host[:port]" | "[v6%scope]:port" | "v6%scope" into host, optional port.
// - host_out: address or hostname (scope kept inside for now; we’ll strip it later for v6).
// Returns true on success; sets *has_port/*port_out when a numeric port is present.
static bool Net_ParseHostPort (const char* in, char* host_out, size_t host_cap, bool* has_port, uint16_t* port_out)
{
	if (has_port)
	{
		*has_port = false;
	}
	if (port_out)
	{
		*port_out = 0;
	}
	if (!in || !*in)
	{
		return false;
	}

	if (*in == '[') { // [v6%scope]:port
		const char* rb = strchr(in, ']');
		if (!rb)
		{
			return false;
		}
		if (!copy_csubstr(in + 1, rb, host_out, host_cap))
		{
			return false; // keep %scope with host
		}
		if (*(rb + 1) == ':' && *(rb + 2) != '\0') {
			uint16 prt = 0;
			if (!parse_port_u16(rb + 2, &prt))
			{
				return false;
			}
			if (port_out)
			{
				*port_out = prt;
			}
			if (has_port)
			{
				*has_port = true;
			}
		}
		return true;
	}

	// Unbracketed: could be v4/hostname[:port] or v6%scope (no port allowed bare)
	const char* last_colon = strrchr(in, ':');
	if (!last_colon) {
		return copy_cstr(in, host_out, host_cap);
	}
	// If there is another colon earlier, treat as IPv6 literal (no port unless bracketed)
	for (const char* p = in; p < last_colon; ++p) {
		if (*p == ':') {
			return copy_cstr(in, host_out, host_cap);
		}
	}
	// host:port
	uint16 prt = 0;
	if (!parse_port_u16(last_colon + 1, &prt))
	{
		return false;
	}
	if (!copy_csubstr(in, last_colon, host_out, host_cap))
	{
		return false;
	}
	if (port_out)
	{
		*port_out = prt;
	}
	if (has_port)
	{
		*has_port = true;
	}
	return true;
}

/*
========================
Net_StringToSockaddr
========================
*/
static bool Net_StringToSockaddr( const char *s, SOCKADDR_STORAGE *sadr, const bool doDNSResolve ) {
	if (!s || !*s || !sadr)
	{
		return false;
	}
	memset(sadr, 0, sizeof(*sadr));

	char host_buf[NI_MAXHOST] = { 0 };
	bool has_port = false;
	uint16 parsed_port = 0;
	if (!Net_ParseHostPort(s, host_buf, sizeof(host_buf), &has_port, &parsed_port))
	{
		return false;
	}

	const uint16 port = has_port ? parsed_port : 0;

	// --- Fast path: numeric IPv4 ------------------------------------------------
	struct in_addr v4 = {};
	memset(&v4, 0, sizeof(v4));
	if (inet_pton(AF_INET, host_buf, &v4) == 1) {
		struct sockaddr_in* sa4 = reinterpret_cast<struct sockaddr_in*>(sadr);
		sa4->sin_family = AF_INET;
		sa4->sin_addr = v4;
		sa4->sin_port = htons(port);
		return true;
	}

	// --- Fast path: numeric IPv6 (with optional %scope) -------------------------
	char v6_only[INET6_ADDRSTRLEN] = { 0 };
	uint32 scope_id = 0;
	if (extract_v6_scope(host_buf, v6_only, sizeof(v6_only), &scope_id)) {
		struct in6_addr v6 = {};
		memset(&v6, 0, sizeof(v6));
		if (inet_pton(AF_INET6, v6_only, &v6) == 1) {
			struct sockaddr_in6* sa6 = reinterpret_cast<struct sockaddr_in6*>(sadr);
			sa6->sin6_family = AF_INET6;
			sa6->sin6_addr = v6;
			sa6->sin6_port = htons(port);
			sa6->sin6_scope_id = scope_id; // important for fe80::/10
			return true;
		}
	}

	// --- DNS fallback (hostnames), if allowed ----------------------------------
	if (!doDNSResolve)
	{
		return false;
	}

	ADDRINFOA hints = {};
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ADDRCONFIG;

	// Service string for port (numeric)
	char svc_buf[6] = { 0 };
	const char* service = nullptr;
	if (port != 0) {
		unsigned val = port;
		char rev[6] = { 0 };
		size_t idx = 0;
		do {
			rev[idx++] = static_cast<char>('0' + (val % 10));
			val /= 10;
		} while (val && idx < 5);

		size_t w = 0;
		for (; w < idx; ++w)
		{
			svc_buf[w] = rev[idx - 1 - w];
		}
		if (w == 0)
		{
			svc_buf[0] = '0'; w = 1;
		}
		svc_buf[w] = '\0';
		service = svc_buf;
		hints.ai_flags |= AI_NUMERICSERV;
	}

	// If the input looked like v6%scope but inet_pton failed (rare), try removing the %scope for DNS
	const char* query = host_buf;
	char host_no_scope[NI_MAXHOST] = { 0 };
	if (strchr(host_buf, '%')) {
		if (extract_v6_scope(host_buf, host_no_scope, sizeof(host_no_scope), NULL))
		{
			query = host_no_scope;
		}
	}

	ADDRINFOA* res = nullptr;
	int gai = getaddrinfo(query, service, &hints, &res);
	if (gai != 0 || !res)
	{
		return false;
	}

	// Pick first AF_INET/AF_INET6; set port if needed; for v6, scope id is determined by system when resolving a hostname.
	bool ok = false;
	for (ADDRINFOA* ai = res; ai; ai = ai->ai_next) {
		if (ai->ai_family == AF_INET && ai->ai_addrlen >= sizeof(struct sockaddr_in)) {
			struct sockaddr_in* sa4 = reinterpret_cast<struct sockaddr_in*>(sadr);
			*sa4 = *reinterpret_cast<const struct sockaddr_in*>(ai->ai_addr);
			if (port != 0)
			{
				sa4->sin_port = htons(port);
			}
			ok = true; break;
		}
		if (ai->ai_family == AF_INET6 && ai->ai_addrlen >= sizeof(struct sockaddr_in6)) {
			struct sockaddr_in6* sa6 = reinterpret_cast<struct sockaddr_in6*>(sadr);
			*sa6 = *reinterpret_cast<const struct sockaddr_in6*>(ai->ai_addr);
			if (port != 0)
			{
				sa6->sin6_port = htons(port);
			}
			// Note: hostname resolves won't carry a %scope; that's fine for global v6.
			ok = true; break;
		}
	}

	if (res)
	{
		freeaddrinfo(res);
	}
	return ok;
}

/*
========================
NET_IPSocket
========================
*/
static SOCKET NET_IPSocket( const char *net_interface, const index_t port, netadr_t *bound_to ) {
	SOCKET				newsocket = 0;
	SOCKADDR_STORAGE	address = {};
	unsigned long		_true = 1;
	const size_t		i = 1;
	int					err = 0;

	if ( port != PORT_ANY ) {
		if( net_interface ) {
			idLib::Printf( "Opening IP socket: %s:%i\n", net_interface, port );
		} else {
			idLib::Printf( "Opening IP socket: localhost:%i\n", port );
		}
	}

	if( ( newsocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
		err = WSAGetLastError();
		if( err != WSAEAFNOSUPPORT ) {
			idLib::Printf( "WARNING: UDP_OpenSocket: socket: %s\n", NET_ErrorString() );
		}
		return 0;
	}

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
		idLib::Printf( "WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	// make it broadcast capable
	char optval[MAX_STRING_CHARS] = {};
	const size_t strSize = idStr::ItoA(optval, sizeof(optval), i);
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, optval, numeric_cast<int>(strSize) ) == SOCKET_ERROR ) {
		idLib::Printf( "WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	if( !net_interface || !net_interface[0] || !idStr::Icmp( net_interface, "localhost" ) ) {
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		Net_StringToSockaddr( net_interface, &address, true );
	}

	if( port == PORT_ANY ) {
		address.sin_port = 0;
	}
	else {
		address.sin_port = htons( numeric_cast<uint16>(port) );
	}

	address.sin_family = AF_INET;

	if( bind( newsocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address) ) == SOCKET_ERROR ) {
		idLib::Printf( "WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	// if the port was PORT_ANY, we need to query again to know the real port we got bound to
	// ( this used to be in idUDP::InitForPort )
	if ( bound_to ) {
		size_t len = numeric_cast<int>(sizeof( address ));
		getsockname( newsocket, reinterpret_cast<sockaddr*>(&address), &len );
		Net_SockadrToNetadr( &address, bound_to );
	}

	return newsocket;
}

/*
========================
NET_OpenSocks
========================
*/
[[maybe_unused]] static void NET_OpenSocks(const index_t port ) {
	sockaddr_in			address = {};
	struct hostent		*h = nullptr;
	int  				len = 0;
	bool				rfc1929 = false;
	unsigned char		buf[64] = {};

	usingSocks = false;

	idLib::Printf( "Opening connection to SOCKS server.\n" );

	if ( ( socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET ) {
		idLib::Printf( "WARNING: NET_OpenSocks: socket: %s\n", NET_ErrorString() );
		return;
	}

	h = getaddrinfo( net_socksServer.GetString() );
	if ( h == nullptr) {
		idLib::Printf( "WARNING: NET_OpenSocks: getaddrinfo: %s\n", NET_ErrorString() );
		return;
	}
	if ( h->h_addrtype != AF_INET ) {
		idLib::Printf( "WARNING: NET_OpenSocks: getaddrinfo: address type was not AF_INET\n" );
		return;
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = *reinterpret_cast<int*>(h->h_addr_list[0]);
	address.sin_port = htons( static_cast<short>(net_socksPort.GetInteger()) );

	if ( connect( socks_socket, reinterpret_cast<sockaddr*>(&address), sizeof( address ) ) == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: connect: %s\n", NET_ErrorString() );
		return;
	}

	// send socks authentication handshake
	if ( *net_socksUsername.GetString() || *net_socksPassword.GetString() ) {
		rfc1929 = true;
	}
	else {
		rfc1929 = false;
	}

	buf[0] = 5;		// SOCKS version
	// method count
	if ( rfc1929 ) {
		buf[1] = 2;
		len = 4;
	}
	else {
		buf[1] = 1;
		len = 3;
	}
	buf[2] = 0;		// method #1 - method id #00: no authentication
	if ( rfc1929 ) {
		buf[2] = 2;		// method #2 - method id #02: username/password
	}
	if ( send( socks_socket, reinterpret_cast<const char*>(buf), len, 0 ) == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, reinterpret_cast<char*>(buf), 64, 0 );
	if ( len == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
		return;
	}
	if ( len != 2 || buf[0] != 5 ) {
		idLib::Printf( "NET_OpenSocks: bad response\n" );
		return;
	}
	switch( buf[1] ) {
	case 0:	// no authentication
	case 2: // username/password authentication
		break;
	default:
		idLib::Printf( "NET_OpenSocks: request denied\n" );
		return;
	}

	// do username/password authentication if needed
	if ( buf[1] == 2 ) {
		size_t	ulen = 0;
		size_t	plen = 0;

		// build the request
		ulen = idStr::Length( net_socksUsername.GetString() );
		plen = idStr::Length( net_socksPassword.GetString() );

		buf[0] = 1;		// username/password authentication version
		buf[1] = numeric_cast<uint8>(ulen);
		if ( ulen ) {
			memcpy( &buf[2], net_socksUsername.GetString(), ulen );
		}
		buf[2 + ulen] = numeric_cast<uint8>(plen);
		if ( plen ) {
			memcpy( &buf[3 + ulen], net_socksPassword.GetString(), plen );
		}

		// send it
		if ( send( socks_socket, reinterpret_cast<const char*>(buf), numeric_cast<int>(3 + ulen + plen), 0 ) == SOCKET_ERROR ) {
			idLib::Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
			return;
		}

		// get the response
		len = recv( socks_socket, reinterpret_cast<char*>(buf), 64, 0 );
		if ( len == SOCKET_ERROR ) {
			idLib::Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
			return;
		}
		if ( len != 2 || buf[0] != 1 ) {
			idLib::Printf( "NET_OpenSocks: bad response\n" );
			return;
		}
		if ( buf[1] != 0 ) {
			idLib::Printf( "NET_OpenSocks: authentication failed\n" );
			return;
		}
	}

	// send the UDP associate request
	buf[0] = 5;		// SOCKS version
	buf[1] = 3;		// command: UDP associate
	buf[2] = 0;		// reserved
	buf[3] = 1;		// address type: IPV4
	*reinterpret_cast<int*>(&buf[4]) = INADDR_ANY;
	*reinterpret_cast<uint16*>(&buf[8]) = htons( numeric_cast<uint16>(port) );		// port
	if ( send( socks_socket, reinterpret_cast<const char*>(buf), 10, 0 ) == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, reinterpret_cast<char*>(buf), 64, 0 );
	if( len == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
		return;
	}
	if( len < 2 || buf[0] != 5 ) {
		idLib::Printf( "NET_OpenSocks: bad response\n" );
		return;
	}
	// check completion code
	if( buf[1] != 0 ) {
		idLib::Printf( "NET_OpenSocks: request denied: %i\n", buf[1] );
		return;
	}
	if( buf[3] != 1 ) {
		idLib::Printf( "NET_OpenSocks: relay address is not IPV4: %i\n", buf[3] );
		return;
	}
	socksRelayAddr.sin_family = AF_INET;
	socksRelayAddr.sin_addr.s_addr = *reinterpret_cast<int*>(&buf[4]);
	socksRelayAddr.sin_port = *reinterpret_cast<uint16*>(&buf[8]);
	memset( socksRelayAddr.sin_zero, 0, sizeof( socksRelayAddr.sin_zero ) );

	usingSocks = true;
}

/*
========================
Net_WaitForData
========================
*/
static bool Net_WaitForData( const SOCKET netSocket, const ID_TIME_T timeout ) {
	int					ret = 0;
	fd_set				set = {};
	struct timeval		tv = {};

	if ( !netSocket ) {
		return false;
	}

	if ( timeout < 0 ) {
		return true;
	}

	FD_ZERO( &set );
	FD_SET( static_cast<unsigned int>( netSocket ), &set );

	tv.tv_sec = 0;
	tv.tv_usec = numeric_cast<long>( timeout * 1000);

	ret = select( numeric_cast<int>(netSocket + 1), &set, nullptr, nullptr, &tv );

	if ( ret == -1 ) {
		char sys_error_msg[MAX_STRING_CHARS] = {};

#if defined(ID_WIN64) || defined(ID_WIN32))
		if (strerror_s(sys_error_msg, sizeof(sys_error_msg), ret) != 0)
		{
			strncpy_s(sys_error_msg, sizeof(sys_error_msg), "Unknown error", 14);
			sys_error_msg[sizeof (sys_error_msg) - 1] = '\0';
		}
#else
		if (strerror_r(err_code, sys_error_msg, sizeof(sys_error_msg)) != 0)
		{
			strncpy_s(sys_error_msg, sizeof(sys_error_msg), "Unknown error", 14);
			sys_error_msg[sizeof sys_error_msg - 1] = '\0';
		}
#endif
		idLib::Printf( "Net_WaitForData select(): %s\n", sys_error_msg );
		return false;
	}

	// timeout with no data
	if ( ret == 0 ) {
		return false;
	}

	return true;
}

/*
========================
Net_GetUDPPacket
========================
*/
static bool Net_GetUDPPacket( const SOCKET netSocket, netadr_t &net_from, char *data, size_t &size, const size_t maxSize ) {
	int 			    ret = 0;
	SOCKADDR_STORAGE	from = {};
	int				    fromlen = 0;
	int				    err = 0;

	if ( !netSocket ) {
		return false;
	}

	fromlen = sizeof(from);
	ret = recvfrom( netSocket, data, numeric_cast<int>(maxSize), 0, reinterpret_cast<sockaddr*>(&from), &fromlen );
	if ( ret == SOCKET_ERROR ) {
		err = WSAGetLastError();

		if ( err == WSAEWOULDBLOCK || err == WSAECONNRESET ) {
			return false;
		}
		char	buf[1024] = {};
		std::ignore = sprintf( buf, "Net_GetUDPPacket: %s\n", NET_ErrorString() );
		idLib::Printf( buf );
		return false;
	}

	if ( netSocket == ip_socket ) {
		memset( from.sin_zero, 0, sizeof( from.sin_zero ) );
	}

	if ( usingSocks && netSocket == ip_socket && memcmp( &from, &socksRelayAddr, fromlen ) == 0 ) {
		if ( ret < 10 || data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1 ) {
			return false;
		}
		net_from.type = NA_IPv4;
		net_from.ip[0] = data[4];
		net_from.ip[1] = data[5];
		net_from.ip[2] = data[6];
		net_from.ip[3] = data[7];
		net_from.port = *reinterpret_cast<short*>(&data[8]);
		memmove( data, &data[10], ret - 10 );
	} else {
		Net_SockadrToNetadr( &from, &net_from );
	}

	if ( std::cmp_greater(ret, maxSize) ) {
		char	buf[1024] = {};
		std::ignore = sprintf( buf, "Net_GetUDPPacket: oversize packet from %s\n", Sys_NetAdrToString( net_from ) );
		idLib::Printf( buf );
		return false;
	}

	size = ret;

	return true;
}

/*
========================
Net_SendUDPPacket
========================
*/
static void Net_SendUDPPacket( const SOCKET netSocket, const size_t length, const void *data, const netadr_t& to ) {
	int				    ret = 0;
	SOCKADDR_STORAGE	addr = {};

	if ( !netSocket ) {
		return;
	}

	Net_NetadrToSockadr( &to, &addr );

	if ( usingSocks && to.type == NA_IPv4 ) {
		socksBuf[0] = 0;	// reserved
		socksBuf[1] = 0;
		socksBuf[2] = 0;	// fragment (not fragmented)
		socksBuf[3] = 1;	// address type: IPV4
		*reinterpret_cast<uint32*>(&socksBuf[4]) = addr.sin_addr.s_addr;
		*reinterpret_cast<uint16*>(&socksBuf[8]) = addr.sin_port;
		memcpy( &socksBuf[10], data, length );
		ret = sendto( netSocket, socksBuf, numeric_cast<int>(length + 10), 0, reinterpret_cast<sockaddr*>(&socksRelayAddr), sizeof(socksRelayAddr) );
	} else {
		ret = sendto( netSocket, static_cast<const char*>(data), numeric_cast<int>(length), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr) );
	}
	if ( ret == SOCKET_ERROR ) {
		const int err = WSAGetLastError();

		// some PPP links do not allow broadcasts and return an error
		if ( ( err == WSAEADDRNOTAVAIL ) && ( to.type == NA_BROADCAST ) ) {
			return;
		}

		// NOTE: WSAEWOULDBLOCK used to be silently ignored,
		// but that means the packet will be dropped so I don't feel it's a good thing to ignore
		idLib::Printf( "UDP sendto error - packet dropped: %s\n", NET_ErrorString() );
	}
}

/*
========================
Sys_PushUnicastNetInterface
========================
*/
static void Sys_PushUnicastNetInterface(net_interface_t* ni, const IP_ADAPTER_UNICAST_ADDRESS* ua) {
	if (!ni || !ua || ni->UnicastCount >= MAX_UNICAST_PER_INTERFACE)
	{
		return;
	}
	net_addr_entry_t* e = &ni->Unicast[ni->UnicastCount++];
	memset(e, 0, sizeof(*e));
	if (ua->Address.lpSockaddr && ua->Address.iSockaddrLength <= sizeof(SOCKADDR_STORAGE)) {
		memcpy(&e->sa, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
		e->prefix_len = ua->OnLinkPrefixLength;
		addr_text_from_sa(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, e->text, sizeof(e->text));
	}
}

/*
========================
Sys_ClearNetInterface
========================
*/
static void Sys_ClearNetInterface(net_interface_t* ni) {
	if (ni)
	{
		memset(ni, 0, sizeof(*ni));
	}
}

/*
========================
Sys_EnumerateNetInterfaces
========================
*/
// Converts NUL-terminated UTF-16 (wchar_t*) to UTF-8 into out[cap].
// Returns true on success (even if truncated). Ensures out is NUL-terminated.
static bool w16_to_utf8(const wchar_t* w, char* out, const size_t cap) {
	if (!out || cap == 0)
	{
		return false;
	}
	out[0] = '\0';
	if (!w || *w == L'\0')
	{
		return true;
	}

	// First pass: required size (includes NUL)
	int need = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w, -1, nullptr, 0, nullptr, nullptr);
	if (need <= 0) {
		// Fallback without strict validation (some old drivers use odd surrogate pairs)
		need = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
		if (need <= 0)
		{
			return false;
		}
	}

	if (numeric_cast<size_t>(need) <= cap) {
		// Fits in buffer
		const int wrote = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, numeric_cast<int>(cap), nullptr, nullptr);
		return (wrote > 0);
	}
	else {
		// Truncate safely: write as much as we can, leave room for NUL
		const int wrote = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, numeric_cast<int>(cap - 1), nullptr, nullptr);
		out[cap - 1] = '\0';
		return (wrote > 0);
	}
}

static size_t Sys_EnumerateNetInterfaces(const int only_up /*1=yes*/, const int skip_loop_and_tunnel /*1=yes*/)
{
	// Zero output slots
	for (auto& networkInterface : networkInterfacesArray)
	{
		Sys_ClearNetInterface(&networkInterface);
	}

	ULONG flags =
		GAA_FLAG_INCLUDE_PREFIX |
		GAA_FLAG_SKIP_ANYCAST |
		GAA_FLAG_SKIP_MULTICAST |
		GAA_FLAG_INCLUDE_GATEWAYS;

	ULONG family = AF_UNSPEC; // v4 + v6
	ULONG bufLen = 15 * 1024;
	IP_ADAPTER_ADDRESSES* addrs = nullptr;

	DWORD ret = 0;
	for (size_t tries = 0; tries < 3; ++tries) 
	{
		addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(bufLen));
		if (!addrs)
		{
			ret = ERROR_NOT_ENOUGH_MEMORY; break;
		}
		memset(addrs, 0, bufLen);

		ret = GetAdaptersAddresses(family, flags, nullptr, addrs, &bufLen);
		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			free(addrs); addrs = nullptr;
			continue;
		}
		break;
	}
	if (ret != NO_ERROR || !addrs) {
		if (addrs)
		{
			free(addrs);
		}
		return 0;
	}

	size_t count = 0;
	for (IP_ADAPTER_ADDRESSES* adapterAddress = addrs; adapterAddress && count < MAX_INTERFACES; adapterAddress = adapterAddress->Next) 
	{
		if (only_up && adapterAddress->OperStatus != IfOperStatusUp)
		{
			continue;
		}
		if (skip_loop_and_tunnel &&	(adapterAddress->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapterAddress->IfType == IF_TYPE_TUNNEL))
		{
			continue;
		}

		net_interface_t* networkInterface = &networkInterfacesArray[count];
		Sys_ClearNetInterface(networkInterface);

		// Identity / state
		networkInterface->IfIndex     = adapterAddress->IfIndex;
		networkInterface->Ipv6IfIndex = adapterAddress->Ipv6IfIndex;
		networkInterface->Luid        = adapterAddress->Luid;
		networkInterface->Mtu         = adapterAddress->Mtu;
		networkInterface->OperStatus  = adapterAddress->OperStatus;
		networkInterface->IfType      = adapterAddress->IfType;
		networkInterface->Flags       = adapterAddress->Flags;

		// Names
		if (adapterAddress->AdapterName) {
			strncpy_s(networkInterface->AdapterName, adapterAddress->AdapterName, sizeof(networkInterface->AdapterName) - 1);
		}
		if (adapterAddress->Description) {
			w16_to_utf8(adapterAddress->Description, networkInterface->Description, sizeof(networkInterface->Description));
		}
		if (adapterAddress->FriendlyName) {
			w16_to_utf8(adapterAddress->FriendlyName, networkInterface->FriendlyName, sizeof(networkInterface->FriendlyName));
		}

		// MAC
		networkInterface->MacLen = (adapterAddress->PhysicalAddressLength > sizeof(networkInterface->Mac)) ? sizeof(networkInterface->Mac) : adapterAddress->PhysicalAddressLength;
		if (networkInterface->MacLen)
		{
			memcpy(networkInterface->Mac, adapterAddress->PhysicalAddress, networkInterface->MacLen);
		}

		// Unicast
		for (IP_ADAPTER_UNICAST_ADDRESS* ua = adapterAddress->FirstUnicastAddress; ua; ua = ua->Next) {
			Sys_PushUnicastNetInterface(networkInterface, ua);
			if (networkInterface->UnicastCount >= NI_MAX_UNICAST)
			{
				break;
			}
		}

		// Gateways (Windows 8+)
#ifdef IP_ADAPTER_GATEWAY_ADDRESS_LH
		for (IP_ADAPTER_GATEWAY_ADDRESS_LH* gw = adapterAddress->FirstGatewayAddress; gw; gw = gw->Next) {
			ni_push_gateway(networkInterface, gw);
			if (networkInterface->GatewayCount >= NI_MAX_GATEWAYS)
			{
				break;
			}
		}
#endif

		// DNS
		for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = adapterAddress->FirstDnsServerAddress; ds; ds = ds->Next) {
			ni_push_dns(networkInterface, ds);
			if (networkInterface->DnsCount >= NI_MAX_DNS)
			{
				break;
			}
		}

		++count;
	}

	free(addrs);
	return count;
}

/*
========================
Sys_InitNetworking
========================
*/
void Sys_InitNetworking() {
	int		r = 0;

	if ( winsockInitialized ) {
		return;
	}
	r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r ) {
		idLib::Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
		return;
	}

	winsockInitialized = true;
	idLib::Printf( "Winsock Initialized\n" );

	PIP_ADAPTER_INFO pAdapterInfo = {};
	PIP_ADAPTER_INFO pAdapter = nullptr;
	DWORD dwRetVal = 0;
	PIP_ADDR_STRING pIPAddrString = {};
	ULONG ulOutBufLen = 0;
	bool foundloopback = false;

	numInterfaces = 0;
	foundloopback = false;

	pAdapterInfo = static_cast<IP_ADAPTER_INFO*>(malloc(sizeof(IP_ADAPTER_INFO)));
	if( !pAdapterInfo ) {
		idLib::FatalError( "Sys_InitNetworking: Couldn't malloc( %d )", sizeof( IP_ADAPTER_INFO ) );
	}
	ulOutBufLen = sizeof( IP_ADAPTER_INFO );

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if( GetAdaptersInfo( pAdapterInfo, &ulOutBufLen ) == ERROR_BUFFER_OVERFLOW ) {
		free( pAdapterInfo );
		pAdapterInfo = static_cast<IP_ADAPTER_INFO*>(malloc(ulOutBufLen)); 
		if( !pAdapterInfo ) {
			idLib::FatalError( "Sys_InitNetworking: Couldn't malloc( %ld )", ulOutBufLen );
		}
	}

	if( ( dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) ) != NO_ERROR ) {
		// happens if you have no network connection
		idLib::Printf( "Sys_InitNetworking: GetAdaptersInfo failed (%ld).\n", dwRetVal );
	} else {
		pAdapter = pAdapterInfo;
		while( pAdapter ) {
			idLib::Printf( "Found interface: %s %s - ", pAdapter->AdapterName, pAdapter->Description );
			pIPAddrString = &pAdapter->IpAddressList;
			while( pIPAddrString ) {
				unsigned long ip_a = 0, ip_m = 0;
				if( !idStr::Icmp( "127.0.0.1", pIPAddrString->IpAddress.String ) ) {
					foundloopback = true;
				}
				inet_pton(AF_INET, pIPAddrString->IpAddress.String, &ip_a);
				ip_a = ntohl( ip_a );
				inet_pton(AF_INET, pIPAddrString->IpMask.String, &ip_m);
				ip_m = ntohl( ip_m );
				//skip null netmasks
				if( !ip_m ) {
					idLib::Printf( "%s NULL netmask - skipped\n", pIPAddrString->IpAddress.String );
					pIPAddrString = pIPAddrString->Next;
					continue;
				}
				idLib::Printf( "%s/%s\n", pIPAddrString->IpAddress.String, pIPAddrString->IpMask.String );
				networkInterfacesArray[numInterfaces].ip = ip_a;
				networkInterfacesArray[numInterfaces].mask = ip_m;
				idStr::Copynz( networkInterfacesArray[numInterfaces].addr, pIPAddrString->IpAddress.String, sizeof( networkInterfacesArray[numInterfaces].addr ) );
				numInterfaces++;
				if( numInterfaces >= MAX_INTERFACES ) {
					idLib::Printf( "Sys_InitNetworking: MAX_INTERFACES(%d) hit.\n", MAX_INTERFACES );
					free( pAdapterInfo );
					return;
				}
				pIPAddrString = pIPAddrString->Next;
			}
			pAdapter = pAdapter->Next;
		}
	}
	// for some reason, win32 doesn't count loopback as an adapter...
	if( !foundloopback && numInterfaces < MAX_INTERFACES ) {
		unsigned long ip_a = 0, ip_m = 0;
		idLib::Printf( "Sys_InitNetworking: adding loopback interface\n" );
		inet_pton(AF_INET, "127.0.0.1", &ip_a);
		networkInterfacesArray[numInterfaces].ip = ntohl( ip_a );
		inet_pton(AF_INET, "255.0.0.0", &ip_m);
		networkInterfacesArray[numInterfaces].mask = ntohl( ip_m );
		numInterfaces++;
	}
	free( pAdapterInfo );
}

/*
========================
Sys_ShutdownNetworking
========================
*/
void Sys_ShutdownNetworking() {
	if ( !winsockInitialized ) {
		return;
	}
	WSACleanup();
	winsockInitialized = false;
}

/*
========================
Sys_StringToNetAdr
========================
*/
bool Sys_StringToNetAdr( const char *s, netadr_t *adr, const bool doDNSResolve ) {
	SOCKADDR_STORAGE sadr = {};
	
	if ( !Net_StringToSockaddr( s, &sadr, doDNSResolve ) ) {
		return false;
	}
	
	Net_SockadrToNetadr( &sadr, adr );
	return true;
}

/*
========================
Sys_NetAdrToString
========================
*/
const char *Sys_NetAdrToString( const netadr_t& adr ) {
	static size_t interface_string_buffer_index = 0;
	static char interface_string_buffer_2D[MAX_INTERFACES][INET6_ADDRSTRLEN] = {};	// flip/flop
	char *s = nullptr;
	char ipBuf[INET6_ADDRSTRLEN] = { 0 };
	const uint16 port = ntohs(adr.port);

	s = interface_string_buffer_2D[interface_string_buffer_index];
	interface_string_buffer_index = (interface_string_buffer_index + 1) % MAX_INTERFACES;

	if ( adr.type == NA_LOOPBACK ) {
		if ( adr.port ) {
			idStr::snPrintf( s, INET6_ADDRSTRLEN, "localhost:%i", adr.port );
		} else {
			idStr::snPrintf( s, INET6_ADDRSTRLEN, "localhost" );
		}
	}
	else if ( adr.type == NA_BROADCAST ) {
		if (adr.port) {
			idStr::snPrintf(s, INET6_ADDRSTRLEN, "broadcast:%i", adr.port);
		}
		else {
			idStr::snPrintf(s, INET6_ADDRSTRLEN, "broadcast");
		}
	}
	else if ( adr.type == NA_IPv4 ) {
		if (!inet_ntop(AF_INET, &adr.addr.ip4, ipBuf, sizeof(ipBuf)))
		{
			return "";
		}

		if (port)
		{
			idStr::snPrintf(s, INET6_ADDRSTRLEN, "%s:%hu", ipBuf, port);
		}
		else
		{
			idStr::snPrintf(s, INET6_ADDRSTRLEN, "%s", ipBuf);
		}
	}
	else if (adr.type == NA_IPv6) {
		if (!inet_ntop(AF_INET6, &adr.addr.ip6, ipBuf, sizeof(ipBuf)))
		{
			return "";
		}

		if (port)
		{
			idStr::snPrintf(s, INET6_ADDRSTRLEN, "[%s]:%hu", ipBuf, port);
		}
		else
		{
			idStr::snPrintf(s, INET6_ADDRSTRLEN, "[%s]", ipBuf);
		}
	}
	return s;
}

/*
========================
Sys_IsLANAddress
========================
*/
bool Sys_IsLANAddress( const netadr_t& adr ) {
	// loopback, link-local, private ranges, ULA, and v4 broadcast count as “LAN-ish”
	if (adr.flags & (NETADR_F_LOOPBACK | 
					NETADR_F_LINKLOCAL |
					NETADR_F_PRIVATE_V4 | 
					NETADR_F_ULA_V6 |
					NETADR_F_BROADCAST_V4))
	{
		return true;
	}

	return false;
}

/*
========================
Sys_CompareNetAdrBase

Compares without the port.
========================
*/
bool Sys_CompareNetAdrBase( const netadr_t& a, const netadr_t& b ) {
	// Treat loopback == loopback regardless of family
	if ((a.flags & NETADR_F_LOOPBACK) && (b.flags & NETADR_F_LOOPBACK))
	{
		return true;
	}

	if (a.type == NA_IPv4 && b.type == NA_IPv4) {
		return memcmp(a.addr.ip4, b.addr.ip4, 4) == 0;
	}
	if (a.type == NA_IPv6 && b.type == NA_IPv6) {
		if (memcmp(a.addr.ip6, b.addr.ip6, 16) != 0)
		{
			return false;
		}
		// Scope must match for link-local/multicast; for global it can be ignored
		if ((a.flags & NETADR_F_LINKLOCAL) || (a.flags & NETADR_F_MULTICAST)) {
			return a.v6_scope_id == b.v6_scope_id;
		}
		return true;
	}
	// legacy broadcast compare (255.255.255.255)
	if (a.type == NA_BROADCAST && b.type == NA_BROADCAST)
	{
		return true;
	}

	idLib::Printf( "Sys_CompareNetAdrBase: bad address type\n" );
	return false;
}

/*
========================
Sys_GetLocalIPCount
========================
*/
size_t	Sys_GetLocalIPCount() {
	return numInterfaces;
}

/*
========================
Sys_GetLocalIP
========================
*/
const char * Sys_GetLocalIP(const index_t i ) {
	if ( ( i < 0 ) || ( i >= numInterfaces ) ) {
		return nullptr;
	}
	return networkInterfacesArray[i].addr;
}

/*
================================================================================================

	idUDP

================================================================================================
*/

/*
========================
idUDP::idUDP
========================
*/
idUDP::idUDP() {
	netSocket = 0;
	memset( &bound_to, 0, sizeof( bound_to ) );
	silent = false;
	packetsRead = 0;
	bytesRead = 0;
	packetsWritten = 0;
	bytesWritten = 0;
}

/*
========================
idUDP::~idUDP
========================
*/
idUDP::~idUDP() {
	Close();
}

/*
========================
idUDP::InitForPort
========================
*/
bool idUDP::InitForPort(const uint16 portNumber ) {
	ORDINAL_CHECK(portNumber, USHRT_MAX);

	netSocket = NET_IPSocket( net_ip.GetString(), portNumber, &bound_to );
	if ( netSocket <= 0 ) {
		netSocket = 0;
		memset( &bound_to, 0, sizeof( bound_to ) );
		return false;
	}

	return true;
}

/*
========================
idUDP::Close
========================
*/
void idUDP::Close() {
	if ( netSocket ) {
		closesocket( netSocket );
		netSocket = 0;
		memset( &bound_to, 0, sizeof( bound_to ) );
	}
}

/*
========================
idUDP::GetPacket
========================
*/
bool idUDP::GetPacket( netadr_t &from, void *data, size_t &size, const size_t maxSize ) {
	bool ret = false;

	while ( true ) {

		ret = Net_GetUDPPacket( netSocket, from, static_cast<char*>(data), size, maxSize );
		if ( !ret ) {
			break;
		}

		packetsRead++;
		bytesRead += size;

		break;
	}

	return ret;
}

/*
========================
idUDP::GetPacketBlocking
========================
*/
bool idUDP::GetPacketBlocking( netadr_t &from, void *data, size_t &size, const size_t maxSize, const ID_TIME_T timeout ) {

	if ( !Net_WaitForData( netSocket, timeout ) ) {
		return false;
	}

	if ( GetPacket( from, data, size, maxSize ) ) {
		return true;
	}

	return false;
}

/*
========================
idUDP::SendPacket
========================
*/
void idUDP::SendPacket( const netadr_t& to, const void *data, const size_t size ) {
	if ( to.type == NA_BAD ) {
		idLib::Warning( "idUDP::SendPacket: bad address type NA_BAD - ignored" );
		return;
	}

	packetsWritten++;
	bytesWritten += size;

	if ( silent ) {
		return;
	}

	Net_SendUDPPacket( netSocket, size, data, to );
}