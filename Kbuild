#
# slingshot base link driver
#
obj-m := $(SBL_NAME).o

$(SBL_NAME)-m := sbl_inst.o \
		 sbl_module.o \
		 sbl_misc.o \
		 sbl_internal.o \
		 sbl_serdes.o \
		 sbl_pml_serdes_op.o \
		 sbl_link.o \
		 sbl_link_an.o \
		 sbl_media.o \
		 sbl_pml.o \
		 sbl_pml_pcs.o \
		 sbl_pml_mac.o \
		 sbl_pml_llr.o \
		 sbl_timers.o \
		 sbl_debug.o \
		 sbl_serdes_fn.o \
		 sbl_test.o \
		 sbl_sbm_serdes_iface.o \
		 sbl_counters.o \
		 sbl_fec.o

ccflags-y :=	-I$(M) 
