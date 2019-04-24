#include "cl_ipa.h"
#include "cl_util.h"
#include "cl_net.h"
#include "cl_incnet.h"
#include "cl_bstream.h"



int cl_ipa::get_ip_iaddr(cl_ip_iaddr& ipi,const char* buf,int size)
{
	if(size<24)
		return -1;
	ipi.protocol = buf[9];
	ipi.snip = *(unsigned int*)(buf+12);
	ipi.dnip = *(unsigned int*)(buf+16);
	ipi.snport = *(unsigned short*)(buf+20);
	ipi.dnport = *(unsigned short*)(buf+22);
	ipi.pk_size = size;
	return 0;
}


void cl_ipa::dump_ippacket(const char* buf,int size)
{
	cl_ip_header h;
	char c;
	unsigned short source_port,dest_port;
	unsigned short len = 0;
	if(size<28)
	{
		DEBUGMSG("#*** wrong ip packet(size=%d);\n",size);
		return;
	}
	cl_bstream b((char*)buf,size,size);
	b >> c;
	h.ihl = c&0x0f;
	h.version = (c&0xf0)>>4;
	b >> h.tos;
	b >> h.tot_len;
	b >> h.id;
	b >> h.frag_off;
	b >> h.ttl;
	b >> h.protocol;
	b >> h.check;
	b >> h.saddr;
	b >> h.daddr;

	b >> source_port;
	b >> dest_port;

	source_port = ntohs(source_port);
	dest_port = ntohs(dest_port);
	if(17==h.protocol)
	{
		b >> len;
		len = ntohs(len);
	}
	printf("{ p%-2d %-15s:%-5d => %-15s:%-5d | %-4d / %-4d ttl=%-3d }\n",(int)h.protocol,
		cl_net::ip_ntoas(h.saddr).c_str(),(int)source_port,cl_net::ip_ntoas(h.daddr).c_str(),(int)dest_port,
		(int)len,size,(int)h.ttl);
	
}

void cl_ipa::dump_ippacket2(const char* buf,int size)
{
	cl_ip_iaddr a;
	if(0==get_ip_iaddr(a,buf,size))
	{
		printf("[%s]-{ p%-2d %-15s :%-5d => %-15s: %-5d |%-4d}\n",
			cl_util::time_to_time_string(time(NULL)).c_str(),
			(int)a.protocol,
			cl_net::ip_ntoas(a.snip).c_str(),(int)ntohs(a.snport),
			cl_net::ip_ntoas(a.dnip).c_str(),(int)ntohs(a.dnport),
			size);
	}
}

