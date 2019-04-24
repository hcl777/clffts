#pragma once



#define	CLYE_ 6000

#define CLYE_CNN_NOFILE				CLYE_+1
#define CLYE_CNN_WRONG_SUBHASH		CLYE_+2
#define CLYE_DOWNBLOCK_WRONG		CLYE_+3

void cly_error_report(int err,const char* msg);


