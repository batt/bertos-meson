
lwip_corefiles =['src/core/init.c',
	'src/core/def.c',
	'src/core/dns.c',
	'src/core/inet_chksum.c',
	'src/core/ip.c',
	'src/core/mem.c',
	'src/core/memp.c',
	'src/core/netif.c',
	'src/core/pbuf.c',
	'src/core/raw.c',
	'src/core/stats.c',
	'src/core/sys.c',
	'src/core/altcp.c',
	'src/core/altcp_alloc.c',
	'src/core/altcp_tcp.c',
	'src/core/tcp.c',
	'src/core/tcp_in.c',
	'src/core/tcp_out.c',
	'src/core/timeouts.c',
	'src/core/udp.c',
]

lwip_core4files = [
    'src/core/ipv4/autoip.c',
	'src/core/ipv4/dhcp.c',
	'src/core/ipv4/etharp.c',
	'src/core/ipv4/icmp.c',
	'src/core/ipv4/igmp.c',
	'src/core/ipv4/ip4_frag.c',
	'src/core/ipv4/ip4.c',
	'src/core/ipv4/ip4_addr.c',
]

lwip_apifiles = [
    'src/api/api_lib.c',
	'src/api/api_msg.c',
	'src/api/err.c',
	'src/api/if_api.c',
	'src/api/netbuf.c',
	'src/api/netdb.c',
	'src/api/netifapi.c',
	'src/api/sockets.c',
	'src/api/tcpip.c',
]

lwip_netiffiles = [
    'src/netif/ethernet.c',
	'src/netif/bridgeif.c',
	'src/netif/bridgeif_fdb.c',
	'src/netif/slipif.c',
]

#-Ibertos/net/lwip/src/include -Ibertos/net/lwip/src/include/ipv4

bertos_lwip_sources = [
    'src/arch/sys_arch.c',
    '../ethernetif.c',
]

lwip_sources = lwip_corefiles + lwip_core4files + lwip_apifiles + lwip_netiffiles + bertos_lwip_sources

lwip_inc = include_directories('src/include')
lwip_dep = declare_dependency(
    sources : files(lwip_sources),
    include_directories: lwip_inc,
)

if 'lwip' in bertos_modules
    project_dependencies += declare_dependency(
        include_directories: lwip_inc,
    )
endif