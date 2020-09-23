#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma region ws2defs

#define WINSOCK_API_LINKAGE __declspec(dllexport)
#define WSAAPI __stdcall

#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128

/*
 * Maximum queue length specifiable by listen.
 */
#define SOMAXCONN       5

//
//  Flags used in "hints" argument to getaddrinfo()
//      - AI_ADDRCONFIG is supported starting with Vista
//      - default is AI_ADDRCONFIG ON whether the flag is set or not
//        because the performance penalty in not having ADDRCONFIG in
//        the multi-protocol stack environment is severe;
//        this defaulting may be disabled by specifying the AI_ALL flag,
//        in that case AI_ADDRCONFIG must be EXPLICITLY specified to
//        enable ADDRCONFIG behavior
//

#define AI_PASSIVE                  0x00000001  // Socket address will be used in bind() call
#define AI_CANONNAME                0x00000002  // Return canonical name in first ai_canonname
#define AI_NUMERICHOST              0x00000004  // Nodename must be a numeric address string
#define AI_NUMERICSERV              0x00000008  // Servicename must be a numeric port number
#define AI_DNS_ONLY                 0x00000010  // Restrict queries to unicast DNS only (no LLMNR, netbios, etc.)

#define AI_ALL                      0x00000100  // Query both IP6 and IP4 with AI_V4MAPPED
#define AI_ADDRCONFIG               0x00000400  // Resolution only if global address configured
#define AI_V4MAPPED                 0x00000800  // On v6 failure, query v4 and convert to V4MAPPED format

#define AI_NON_AUTHORITATIVE        0x00004000  // LUP_NON_AUTHORITATIVE
#define AI_SECURE                   0x00008000  // LUP_SECURE
#define AI_RETURN_PREFERRED_NAMES   0x00010000  // LUP_RETURN_PREFERRED_NAMES

#define AI_FQDN                     0x00020000  // Return the FQDN in ai_canonname
#define AI_FILESERVER               0x00040000  // Resolving fileserver name resolution
#define AI_DISABLE_IDN_ENCODING     0x00080000  // Disable Internationalized Domain Names handling
#define AI_EXTENDED                 0x80000000      // Indicates this is extended ADDRINFOEX(2/..) struct
#define AI_RESOLUTION_HANDLE        0x40000000  // Request resolution handle

/*
 * Protocols
 */
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* group management protocol */
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */

#define IPPROTO_RAW             255             /* raw IP packet */
#define IPPROTO_MAX             256

/*
 * This is used instead of -1, since the
 * SOCKET type is unsigned.
 */
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

 /*
  * Types
  */
#define SOCK_STREAM     1               /* stream socket */
#define SOCK_DGRAM      2               /* datagram socket */
#define SOCK_RAW        3               /* raw-protocol interface */
#define SOCK_RDM        4               /* reliably-delivered message */
#define SOCK_SEQPACKET  5               /* sequenced packet stream */

/*
 * WinSock 2 extension -- manifest constants for shutdown()
 */
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

 /*
  * Address families.
  */
#define AF_UNSPEC       0               /* unspecified */
#define AF_UNIX         1               /* local to host (pipes, portals) */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3               /* arpanet imp addresses */
#define AF_PUP          4               /* pup protocols: e.g. BSP */
#define AF_CHAOS        5               /* mit CHAOS protocols */
#define AF_IPX          6               /* IPX and SPX */
#define AF_NS           6               /* XEROX NS protocols */
#define AF_ISO          7               /* ISO protocols */
#define AF_OSI          AF_ISO          /* OSI is ISO */
#define AF_ECMA         8               /* european computer manufacturers */
#define AF_DATAKIT      9               /* datakit protocols */
#define AF_CCITT        10              /* CCITT protocols, X.25 etc */
#define AF_SNA          11              /* IBM SNA */
#define AF_DECnet       12              /* DECnet */
#define AF_DLI          13              /* Direct data link interface */
#define AF_LAT          14              /* LAT */
#define AF_HYLINK       15              /* NSC Hyperchannel */
#define AF_APPLETALK    16              /* AppleTalk */
#define AF_NETBIOS      17              /* NetBios-style addresses */
#define AF_VOICEVIEW    18              /* VoiceView */
#define AF_FIREFOX      19              /* FireFox */
#define AF_UNKNOWN1     20              /* Somebody is using this! */
#define AF_BAN          21              /* Banyan */

#define AF_MAX          22

#pragma endregion

typedef UINT_PTR        SOCKET;
typedef USHORT ADDRESS_FAMILY;

typedef struct addrinfo
{
	int                 ai_flags;       // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
	int                 ai_family;      // PF_xxx
	int                 ai_socktype;    // SOCK_xxx
	int                 ai_protocol;    // 0 or IPPROTO_xxx for IPv4 and IPv6
	size_t              ai_addrlen;     // Length of ai_addr
	char* ai_canonname;   // Canonical name for nodename
	_Field_size_bytes_(ai_addrlen) struct sockaddr* ai_addr;        // Binary address
	struct addrinfo* ai_next;        // Next structure in linked list
} ADDRINFOA, * PADDRINFOA;

#pragma region MainFuncs

WINSOCK_API_LINKAGE
INT
WSAAPI
getaddrinfo(
	_In_opt_        PCSTR               pNodeName,
	_In_opt_        PCSTR               pServiceName,
	_In_opt_        const ADDRINFOA* pHints,
	_Outptr_        PADDRINFOA* ppResult
);

WINSOCK_API_LINKAGE
_Must_inspect_result_
SOCKET
WSAAPI
socket(
	_In_ int af,
	_In_ int type,
	_In_ int protocol
);

WINSOCK_API_LINKAGE
int
WSAAPI
shutdown(
	_In_ SOCKET s,
	_In_ int how
);

WINSOCK_API_LINKAGE
int
WSAAPI
closesocket(
	_In_ SOCKET s
);

WINSOCK_API_LINKAGE
int
WSAAPI
connect(
	_In_ SOCKET s,
	_In_reads_bytes_(namelen) const struct sockaddr FAR* name,
	_In_ int namelen
);

WINSOCK_API_LINKAGE
_Must_inspect_result_
SOCKET
WSAAPI
accept(
	_In_ SOCKET s,
	_Out_writes_bytes_opt_(*addrlen) struct sockaddr FAR* addr,
	_Inout_opt_ int FAR* addrlen
);

WINSOCK_API_LINKAGE
int
WSAAPI
send(
	_In_ SOCKET s,
	_In_reads_bytes_(len) const char FAR* buf,
	_In_ int len,
	_In_ int flags
);

WINSOCK_API_LINKAGE
int
WSAAPI
recv(
	_In_ SOCKET s,
	_Out_writes_bytes_to_(len, return) __out_data_source(NETWORK) char FAR* buf,
	_In_ int len,
	_In_ int flags
);

#pragma endregion

#pragma region WSAFuncs

WINSOCK_API_LINKAGE
int
WSAAPI
WSAGetLastError(
	void
);

WINSOCK_API_LINKAGE
_Must_inspect_result_
int
WSAAPI
WSAStartup(
	_In_ WORD wVersionRequested,
	_Out_ LPWSADATA lpWSAData
);

#pragma endregion