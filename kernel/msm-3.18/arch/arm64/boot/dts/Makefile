dts-dirs += apm
dts-dirs += arm
dts-dirs += cavium
ifneq (,$(filter if_s200n if_s700n if_mkt200,$(TARGET_PRODUCT)))
dts-dirs += $(TARGET_PRODUCT)
else
dts-dirs += qcom
endif
subdir-y	:= $(dts-dirs)
