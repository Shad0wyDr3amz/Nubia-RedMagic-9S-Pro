# ZTE_MODIFY start for exfat tools by yangshentao
ifeq (opensource,$(ZTE_EXFAT_USES))
PRODUCT_PACKAGES += \
    mount.exfat \
    fsck.exfat \
    mkfs.exfat
endif
# ZTE_MODIFY start for exfat tools by yangshentao
ifeq (PARAGON,$(ZTE_EXFAT_USES))

ifeq ($(TARGET_DEVICE), sdm660_64)
PRODUCT_PACKAGES += \
    jnl.ko \
    ufsd.ko \
    ufsd \
    sysdump
else
PRODUCT_PACKAGES += \
    jnl.ko \
    ufsd.ko \
    chkufsd \
    mkexfat
endif
endif
