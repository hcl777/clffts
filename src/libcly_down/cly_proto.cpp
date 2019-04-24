#include "cly_proto.h"


int operator << (cl_ptlstream& ps, const clyp_head_t& inf)
{
	ps.write(CLY_PTL_STX,4);
	ps << inf.size;
	ps << inf.cmd;
	ps.write_string(inf.hash);
	return ps.ok();
}
int operator >> (cl_ptlstream& ps, clyp_head_t& inf)
{
	ps.read(inf.stx,4);
	ps >> inf.size;
	ps >> inf.cmd;
	ps.read_string(inf.hash,128);
	return ps.ok();
}

int operator << (cl_ptlstream& ps, const clyp_rsp_subtable_t& inf)
{
	ps << inf.finitype;
	ps.write_string(inf.subhash);
	ps << inf.bitsize;
	if(inf.bitsize>0)
		ps.write(inf.bt_buf,(inf.bitsize+7)>>3);
	return ps.ok();
}
int operator >> (cl_ptlstream& ps, clyp_rsp_subtable_t& inf)
{
	ps >> inf.finitype;
	ps.read_string(inf.subhash,1024);
	ps >> inf.bitsize;
	if(inf.bitsize>0)
		ps.read(inf.bt_buf,(inf.bitsize+7)>>3);
	return ps.ok();
}

int operator << (cl_ptlstream& ps, const clyp_blocks_t& inf)
{
	ps << inf.num;
	if(inf.num)
	{
		ps.write_array(inf.indexs,inf.num);
		ps.write_array(inf.offsets,inf.num);
	}
	return ps.ok();
}
int operator >> (cl_ptlstream& ps, clyp_blocks_t& inf)
{
	ps >> inf.num;
	if(inf.num>0)
	{
		ps.read_array(inf.indexs,inf.num);
		ps.read_array(inf.offsets,inf.num);
	}
	return ps.ok();
}
int operator << (cl_ptlstream& ps, const clyp_block_t& inf)
{
	ps << inf.index;
	ps << inf.offset;
	ps << inf.size;
	ps.write(inf.pdata,inf.size);
	return ps.ok();
}
int operator >> (cl_ptlstream& ps, clyp_block_t& inf)
{
	ps >> inf.index;
	ps >> inf.offset;
	ps >> inf.size;
	inf.pdata = ps.read_ptr();
	ps.skipr(inf.size);
	return ps.ok();
}

