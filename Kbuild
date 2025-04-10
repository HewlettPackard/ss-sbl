#
# slingshot base link driver
#
obj-m := $(SBL_NAME).o

ifeq ($(PLATFORM_CASSINI_HW),1)
$(SBL_NAME)-m += nic/sbl_pml_pcs_nic.o \
                 nic/sbl_module_nic.o \
		 nic/sbl_link_nic.o \
		 nic/sbl_media_nic.o \
		 nic/sbl_inst_nic.o \
		 nic/sbl_sbm_serdes_nic.o \
		 nic/sbl_fec_nic.o \
		 nic/sbl_link_an_nic.o \
		 nic/sbl_pml_llr_nic.o
else ifeq ($(PLATFORM_ROSETTA_HW),1)
$(SBL_NAME)-m += sw/sbl_module_sw.o \
		 sw/sbl_link_an_sw.o \
		 sw/sbl_fec_sw.o \
		 sw/sbl_inst_sw.o \
		 sw/sbl_sbm_serdes_sw.o \
		 sw/sbl_pml_llr_sw.o
else
$(error Platform not defined! Define one of PLATFORM_ROSETTA_HW PLATFORM_CASSINI_HW)
endif


$(SBL_NAME)-m += sbl_pml_pcs.o \
		 sbl_link.o \
		 sbl_link_an.o \
		 sbl_media.o \
		 sbl_pml.o \
		 sbl_pml_mac.o \
		 sbl_pml_llr.o \
		 sbl_timers.o \
		 sbl_debug.o \
		 sbl_serdes_fn.o \
		 sbl_test.o \
		 sbl_sbm_serdes.o \
		 sbl_counters.o \
		 sbl_fec.o \
		 sbl_serdes.o \
		 sbl_inst.o \
		 sbl_pml_serdes_op.o \
		 sbl_misc.o \
		 sbl_internal.o

ccflags-y :=	-I$(M)


ifeq ($(PLATFORM_CASSINI_HW),1)
ccflags-y +=	-I$(M)/nic
else
ccflags-y +=	-I$(M)/sw
endif
