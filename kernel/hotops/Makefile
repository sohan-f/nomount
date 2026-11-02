obj-m += nm_hotops_sample.o

NOMOUNT_SRC := $(src)/../src
ccflags-y += -std=gnu11 -Wno-declaration-after-statement -O3 -fno-stack-protector -I$(NOMOUNT_SRC)
ccflags-y += -Wno-unused-variable -Wno-unused-function

ifeq ($(shell grep -qE '\(\*[[:space:]]*__get\)' $(srctree)/include/linux/xattr.h 2>/dev/null && echo 1 || echo 0),1)
ccflags-y += -DNM_HAS_XATTR__GET
endif
