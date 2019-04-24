#makefile:

#################################
CC := gcc
CPP := g++
AR := ar

# release版本不要加 -g 
CFLAGS	= -Wall -g -Werror -DLINUX32 -DNDEBUG -D_FILE_OFFSET_BITS=64
LIBS    = -lpthread -lrt

LOCAL_PATH = ../../..
MY_SPATH_1 := $(LOCAL_PATH)/comm/src/libcl_util
MY_SPATH_2 := $(LOCAL_PATH)/clyun/src/test

#
MY_SRC_LIST := $(wildcard  $(MY_SPATH_1)/*.cpp  $(MY_SPATH_1)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_2)/*.cpp  $(MY_SPATH_2)/*.c)
#

#去掉路径
LOCAL_SRC_FILES := $(notdir $(MY_SRC_LIST))

outpath := ./bin/
output := $(outpath)test
sources :=  $(LOCAL_SRC_FILES)
objects := $(subst .c,.o,$(subst .cpp,.o,$(sources)))
depends := $(subst .c,.d,$(subst .cpp,.d,$(sources)))


INCS := -I$(MY_SPATH_1) -I$(MY_SPATH_2)

####寻找目标文件和依赖文件的路径
VPATH := $(subst -I, , $(INCS)) 

################################
all: $(output)

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INCS)
	
%.o:%.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< $(INCS)

$(output): $(objects)
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
