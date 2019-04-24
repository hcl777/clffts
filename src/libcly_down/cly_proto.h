#pragma once
#include "cl_bstream.h"

#define CLYP_MAX_BLOCKDATA_SIZE 10240
//包标志
#define   CLY_PTL_STX   "clsu"
#define   CLY_PTL_STX_32  *(const uint32*)CLY_PTL_STX
enum ECLY_UTYPE{ CLY_UTYPE_UNKOWN=0,CLY_UTYPE_SERVER,CLY_UTYPE_CLIENT };

//包命令类型
typedef unsigned char cly_cmd_t;
static const cly_cmd_t	CLYP_	= 20;
#define CLYCMD(name,n) static const cly_cmd_t name = CLYP_ + n
CLYCMD(CLYP_REQ_SUBTABLE	,1); //no data
CLYCMD(CLYP_RSP_SUBTABLE	,2);
CLYCMD(CLYP_REQ_BLOCKS		,3);
CLYCMD(CLYP_CANCEL_BLOCKS	,4); //双向都可能发
CLYCMD(CLYP_RSP_BLOCK_DATA	,5); 

typedef struct tag_clyp_head
{
	char		stx[4];
	uint32		size; //含头部大小
	uchar		cmd;
	char		hash[128];
}clyp_head_t;

//CLYP_RSP_SUBTABLE
typedef struct tag_clyp_rsp_subtable
{
	sint32			finitype;//-1=没有文件,0:下载中,1=已完成
	char			subhash[1024];
	sint32			bitsize;	//finitype=1的时候bitsize=0
	char			bt_buf[16];
}clyp_rsp_subtable_t;

//CLYP_REQ_BLOCKS,CLYP_CANCEL_BLOCKS
typedef struct tag_clyp_blocks
{
	uint32			num;
	uint32			indexs[32]; //
	uint32			offsets[32];
}clyp_blocks_t;

//CLYP_RSP_BLOCK_DATA
typedef struct tag_clyp_block
{
	uint32			index; //
	uint32			offset;
	uint32			size;
	char*			pdata;
}clyp_block_t;

int operator << (cl_ptlstream& ps, const clyp_head_t& inf);
int operator >> (cl_ptlstream& ps, clyp_head_t& inf);

int operator << (cl_ptlstream& ps, const clyp_rsp_subtable_t& inf);
int operator >> (cl_ptlstream& ps, clyp_rsp_subtable_t& inf);

int operator << (cl_ptlstream& ps, const clyp_blocks_t& inf);
int operator >> (cl_ptlstream& ps, clyp_blocks_t& inf);

int operator << (cl_ptlstream& ps, const clyp_block_t& inf);
int operator >> (cl_ptlstream& ps, clyp_block_t& inf);

