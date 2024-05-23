all : mini-rv32ima mini-rv32ima.flt

mini-rv32ima : mini-rv32ima.c mini-rv32ima.h default64mbdtc.h
	# for debug
	${CC} -o $@ $< -g -O2 -Wall
	${CC} -o $@-tiny $< -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -fwhole-program -s

NM ?= nm

CFLAGS := -ffunction-sections -fdata-sections -Wall -Wstrict-prototypes -DCNFG_WINDOWS_DISABLE_BATCH
LIBS := -lfdt
SRCS := plic.c virtio.c full-rv32ima.c mmio.c pl011.c stdio.c virtio-net.c
ifeq ($(BLOCK),1)
  SRCS += virtio-blk.c
  CFLAGS += -DWITH_BLOCK
endif
ifeq ($(INPUT),1)
  SRCS += virtio-input.c
  CFLAGS += -DWITH_INPUT
endif

ifeq ($(WINDOWS),1)
  LIBS += -lgdi32 -lwinmm
endif

ifeq ($(LINUX),1)
  LIBS += -lasound
  SRCS += network_tap.c
endif

ifeq ($(X11),1)
  CFLAGS += -DWITH_X11
  LIBS += -lX11
  ifeq ($(STATIC),1)
    LIBS += -lxcb -lXau -lXdmcp
  endif
endif

ifeq ($(SOUND),1)
  CFLAGS += -DWITH_SND
  SRCS += virtio-snd.c
  LIBS += -lasound
endif

LFLAGS := $(LIBS) -Wl,--gc-sections
OBJ := $(addsuffix .o, $(basename $(SRCS)))

ifeq ($(D),1)
CFLAGS += -g -Og -fno-inline-small-functions
else
CFLAGS += -O2
endif

ifeq ($(HTTP),1)
CFLAGS += -DCNFGHTTP
endif

ifeq ($(GL),1)
CFLAGS += -DCNFGOGL
endif

full-rv32ima: $(OBJ) Makefile
	${CC} -o $@ $(OBJ) $(LFLAGS)
	mv new-sizes old-sizes || true
	${NM} -S --size-sort $@ | cut -d' ' -f2- > new-sizes
	diff -u --color old-sizes new-sizes || true

virtio-input.o: virtio-input.c virtio.h x11-key-lut.h plic.h
virtio-blk.o: virtio-blk.c virtio.h
virtio.o: virtio.c virtio.h plic.h
plic.o: plic.c plic.h
full-rv32ima.o: full-rv32ima.c default64mbdtc.h rawdraw_sf.h virtio.h plic.h mmio.h pl011.h stdio.h mini-rv32ima.h
pl011.o: pl011.c pl011.h plic.h mmio.h stdio.h
stdio.o: stdio.c stdio.h
virtio-snd.o: virtio-snd.c virtio.h plic.h
virtio-net.o: virtio-net.c virtio.h plic.h network.h

amalgamation.c: $(SRCS)
	cat $(SRCS) > $@

full-rv32ima.wholeprog: amalgamation.c default64mbdtc.h rawdraw_sf.h virtio.h plic.h x11-key-lut.h
	${CC} -o $@ $< $(CFLAGS) $(LFLAGS) -fwhole-program
	mv new-sizes.wp old-sizes.wp || true
	nm -S --size-sort $@ | cut -d' ' -f2- > new-sizes.wp
	diff -ui --color old-sizes.wp new-sizes.wp || true

%.o: %.c
	${CC} -c -o $@ $< ${CFLAGS}

gen-x11-lut: gen-x11-lut.c
	gcc -o $@ $< -Wall

x11-key-lut.h: gen-x11-lut
	./gen-x11-lut > $@.temp
	mv $@.temp $@

mini-rv32ima.flt : mini-rv32ima.c mini-rv32ima.h
	../buildroot/output/host/bin/riscv32-buildroot-linux-uclibc-gcc -O4 -funroll-loops -s -march=rv32ima -mabi=ilp32 -fPIC $< -Wl,-elf2flt=-r -o $@

# Deply with:  make clean all && cp mini-rv32ima.flt ../buildroot/output/target/root/ && make -C .. toolchain && make testkern

profile : Image.ProfileTest
	echo "#!/bin/sh" > ../buildroot/output/target/etc/init.d/SLocal
	echo 'if [ "$$1" = "start" ]; then /root/mini-rv32ima.flt -f /root/Image.ProfileTest -plt 4 -m 0xfff000; poweroff; fi' >> ../buildroot/output/target/etc/init.d/SLocal
	chmod +x ../buildroot/output/target/etc/init.d/SLocal
	cp Image.ProfileTest ../buildroot/output/target/root/
	make clean all && cp mini-rv32ima.flt ../buildroot/output/target/root/ && make -C .. toolchain && ./mini-rv32ima -f ../buildroot/output/images/Image -plt 4 -m 0x6000000
	echo "Be sure to record the immediately above value."

Image.ProfileTest :
	wget https://github.com/cnlohr/mini-rv32ima-images/raw/master/images/Image.ProfileTest-linux-5.18.0-rv32nommu.zip
	unzip Image.ProfileTest-linux-5.18.0-rv32nommu.zip

Image-emdoom-MAX_ORDER_14 :
	wget https://github.com/cnlohr/mini-rv32ima-images/raw/master/images/Image-emdoom-MAX_ORDER_14.zip
	unzip Image-emdoom-MAX_ORDER_14.zip

testdoom : Image-emdoom-MAX_ORDER_14 mini-rv32ima
	./mini-rv32ima -f Image-emdoom-MAX_ORDER_14 -m 0x3000000 -k "console=ttyS0"

testkern : mini-rv32ima
	./mini-rv32ima -f ../buildroot/output/images/Image -m 0x6000000 -k "console=hvc0"

testbare : mini-rv32ima
	./mini-rv32ima -f ../baremetal/baremetal.bin

DownloadedImage :
	wget https://github.com/cnlohr/mini-rv32ima-images/raw/master/images/linux-6.1.14-rv32nommu-cnl-1.zip -O linux-6.1.14-rv32nommu-cnl-1.zip
	unzip linux-6.1.14-rv32nommu-cnl-1.zip
	mv Image DownloadedImage

testdlimage : mini-rv32ima DownloadedImage
	./mini-rv32ima -f DownloadedImage

# For dumping specific binaries and info.
# SBI is not currently working
#testsbi : mini-rv32ima
#	./mini-rv32ima -f ../opensbi/this_opensbi/platform/riscv_emufun/firmware/fw_payload.bin
#dumpsbi : 
#	../buildroot/output/host/bin/riscv32-buildroot-linux-uclibc-objdump -S ../opensbi/this_opensbi/platform/riscv_emufun/firmware/fw_payload.elf >fw_payload.S

# For converting the .dtb into a .h file for embeddding.
bintoh :
	echo "#include <stdio.h>" > bintoh.c
	echo "int main(int argc,char ** argv) {if(argc==1) return -1; int c, p=0; printf( \"static const unsigned char %s[] = {\", argv[1] ); while( ( c = getchar() ) != EOF ) printf( \"0x%02x,%c\", c, (((p++)&15)==15)?10:' '); printf( \"};\" ); return 0; }" >> bintoh.c
	gcc bintoh.c -o bintoh

default64mbdtc.h : sixtyfourmb.dtb bintoh
	./bintoh default64mbdtb < $< > $@
	# WARNING: sixtyfourmb.dtb MUST hvave at least 16 bytes of buffer room AND be 16-byte aligned.
	#  dtc -I dts -O dtb -o sixtyfourmb.dtb sixtyfourmb.dts -S 1536

sixtyfourmb.dtb : sixtyfourmb.dts
	dtc -I dts -O dtb -o $@ $^ -S 1536


dumpkern :
	../buildroot/output/host/bin/riscv32-buildroot-linux-uclibc-objdump -S ../buildroot/output/build/linux-5.18/vmlinux >fw_payload.S
	../buildroot/output/host/bin/riscv32-buildroot-linux-uclibc-objdump -t ../buildroot/output/build/linux-5.18/vmlinux >fw_payload.t

clean :
	rm -rf mini-rv32ima mini-rv32ima.flt

