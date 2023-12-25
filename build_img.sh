if [ $# -ne 3 ] ;then
        echo "usage: $0 \$(IMAGE) \$(OBJDIR) \$(OSBOOT_START_OFFSET)"
        exit 1
fi

IMAGE=$1
OBJDIR=$2
OSBOOT_START_OFFSET=$3

cp ./hd/test1.img ${IMAGE}
dd if=${OBJDIR}/boot/mbr.bin of=${IMAGE} bs=1 count=446 conv=notrunc

loop_device=`losetup -f`
sudo losetup -P ${loop_device} ${IMAGE}
sudo mkfs.vfat -F 32 ${loop_device}p1
dd if=${OBJDIR}/boot/boot.bin of=${IMAGE} bs=1 count=420 seek=${OSBOOT_START_OFFSET} conv=notrunc

mkdir -p iso
sudo mount ${loop_device}p1 iso/
sudo cp ${OBJDIR}/boot/loader.bin iso/ -v
sudo cp ${OBJDIR}/kernel/kernel.bin iso/ -v
sudo umount iso/

sudo losetup -d ${loop_device}