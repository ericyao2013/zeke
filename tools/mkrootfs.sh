#!/bin/bash

export MTOOLSRC=tools/mtools.conf

if [ "$#" -ne 2 ]; then
    echo "Illegal number of arguments"
    exit 1
fi

# Sector size is 512
IMG=$1
BOOT_FILES=$(cat "$2")
PADDING_SECTORS=8192
BOOTFS_SECTORS=65536
ROOTFS_SECTORS=114688
HOME_SECTORS=335872

let "SECTORS = PADDING_SECTORS + BOOTFS_SECTORS + ROOTFS_SECTORS + HOME_SECTORS"
let "BOOTFS_BEGIN = PADDING_SECTORS"
let "ROOTFS_BEGIN = BOOTFS_BEGIN + BOOTFS_SECTORS"
let "HOME_BEGIN = ROOTFS_BEGIN + ROOTFS_SECTORS"

function dir2img {
    local manifest="$(cat "$1/manifest")"
    if [ -z "$manifest" ]; then
        return 0
    fi
    local files=$(echo "$manifest" | sed "s|[^ ]*|$1/\0|g")

    mmd -D sS "D:$(echo $1 | sed 's|^\./||')"
    for file in $files; do
        local d=$(dirname $file | sed 's|^\./||')
        local destname="$d/$(basename $file)"
        mmd -D sS "D:$d"
        echo "INSTALL $destname"
        mcopy -s $file "D:$destname"
    done
}

# Create the file system image
# C is the boot partition
# D is the system partition
# E is the /home partition
dd if=/dev/zero of="$IMG" bs=512 count=$SECTORS
mpartition -I C:
mpartition -c -b $BOOTFS_BEGIN -l $BOOTFS_SECTORS C:
mpartition -c -b $ROOTFS_BEGIN -l $ROOTFS_SECTORS D:
mpartition -c -b $HOME_BEGIN -l $HOME_SECTORS E:
mformat C:
mformat D:
mformat E:

# Copy the bootloader and kernel image
for file in $BOOT_FILES; do
    mcopy -s "$file" C:
    basename "$file"
done

# Create dirs
mmd D:boot
mmd D:dev
mmd D:home
mmd D:mnt
mmd D:opt
mmd D:opt/test
mmd D:proc
mmd D:root
mmd D:tmp
mmd D:usr
mmd E:hbp # TODO create homedirs more dynamically
mcopy README.md D:

# Copy binaries
MANIFEST=$(find . -name manifest -exec dirname {} \;)
for dir in $MANIFEST; do
    dir2img "$dir"
done

# Give a permission to gain capabilities
mattrib +s 'D:/bin/login'
mattrib +s 'D:/sbin/getty'
mattrib +s 'D:/sbin/sinit'
