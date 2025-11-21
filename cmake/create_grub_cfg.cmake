# Script to create grub.cfg for i386 ISO
file(WRITE "${GRUB_CFG_PATH}" "menuentry \"horizon\" {\n")
file(APPEND "${GRUB_CFG_PATH}" "\tmultiboot /boot/horizon.kernel\n")
file(APPEND "${GRUB_CFG_PATH}" "}\n")
