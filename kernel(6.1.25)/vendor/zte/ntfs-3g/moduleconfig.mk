# ZTE_MODIFY start for NTFS tools by yangshentao
ifeq (true,$(ZTE_NTFS_SUPPORT))
PRODUCT_PACKAGES += \
    fsck.ntfs \
    mkfs.ntfs \
    mount.ntfs
endif
# ZTE_MODIFY end for NTFS tools by yangshentao
