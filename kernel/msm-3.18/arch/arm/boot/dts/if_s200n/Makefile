dtb-y += $(TARGET_PRODUCT).dtb

ifeq ($(TARGET_PRODUCT),if_s200n)
dtb-y += if_s200n_r2.dtb
dtb-y += if_s200n_r3.dtb
dtb-y += if_s200n_r4.dtb
endif

ifeq ($(CONFIG_ARM64),y)
always          := $(dtb-y)
subdir-y        := $(dts-dirs)
else
targets += dtbs
targets += $(addprefix ../, $(dtb-y))

$(obj)/../%.dtb: $(src)/%.dts FORCE
        $(call if_changed_dep,dtc)

dtbs: $(addprefix $(obj)/,$(dtb-y))
endif
clean-files := *.dtb
