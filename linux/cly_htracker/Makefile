#makefile:

#################################
CC := gcc
CPP := g++
AR := ar

# release版本不要加 -g 
CFLAGS	= -Wall -Werror -g -DLINUX32 -DNDEBUG -D_FILE_OFFSET_BITS=64
LIBS    = -lpthread -lrt -ldl

LOCAL_PATH = ../../..
MY_SPATH_1 := $(LOCAL_PATH)/comm/src/libcl_util
MY_SPATH_2 := $(LOCAL_PATH)/comm/src/libhttpsvr
MY_SPATH_3 := $(LOCAL_PATH)/comm/src/libcl_db
MY_SPATH_4 := $(LOCAL_PATH)/clyun/src/libcly_tracker
MY_SPATH_5 := $(LOCAL_PATH)/clyun/src/cly_htracker

#
MY_SRC_LIST := $(wildcard  $(MY_SPATH_1)/*.cpp  $(MY_SPATH_1)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_2)/*.cpp  $(MY_SPATH_2)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_3)/*.cpp  $(MY_SPATH_3)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_4)/*.cpp  $(MY_SPATH_4)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_5)/*.cpp  $(MY_SPATH_5)/*.c)
#

#去掉路径
LOCAL_SRC_FILES := $(notdir $(MY_SRC_LIST))

outpath := ./bin/
output := $(outpath)cly_htracker
sources :=  $(LOCAL_SRC_FILES)
objects := $(subst .c,.o,$(subst .cpp,.o,$(sources)))
depends := $(subst .c,.d,$(subst .cpp,.d,$(sources)))

INCS := -I$(MY_SPATH_1) -I$(MY_SPATH_2) -I$(MY_SPATH_3) -I$(MY_SPATH_4) -I$(MY_SPATH_5)

####### mysql...
# 32位 _M_IX86  ； x86_64 用 _M_X64 或 __LP64
ifeq ($(shell getconf LONG_BIT), 64)
INCS		+= -I../../../../source/mysql/c/linux/mysql-connector-c-6.1.9-linux-glibc2.5-x86_64/include
archives	:= ../../../../source/mysql/c/linux/mysql-connector-c-6.1.9-linux-glibc2.5-x86_64/lib/libmysqlclient.a
else
INCS		+= -I../../../../source/mysql/c/linux/mysql-connector-c-6.1.9-linux-glibc2.5-i686/include
archives	:= ../../../../source/mysql/c/linux/mysql-connector-c-6.1.9-linux-glibc2.5-i686/lib/libmysqlclient.a
endif


####寻找目标文件和依赖文件的路径
VPATH := $(subst -I, , $(INCS)) 

################################
all: $(output)

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INCS)
	
%.o:%.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< $(INCS)

$(output): $(objects) $(archives)
	-mkdir $(outpath)
	$(CPP) -o $@ $^ $(LIBS)

#################################
#depend:
%.d: %.c
	@set -e; rm -f $@;			\
	$(CC) -MM $(CFLAGS) $(INCS) $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;   \
	rm -f $@.$$$$
	
%.d: %.cpp
	@set -e; rm -f $@;			\
	$(CPP) -MM $(CPPFLAGS) $(INCS) $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;   \
	rm -f $@.$$$$

include $(depends)		
#################################
.PHONY : print clean
print:
	#echo $(sources)
	#echo $(objects)
	#echo $(depends)
	#echo $(CFLAGS)
	#echo $(CPPFLAGS)
	#echo $(VPATH)
			
clean:
	-rm -rf *.o
	-rm *.d
	-rm *.d.*
	-rm $(output)
##################################
