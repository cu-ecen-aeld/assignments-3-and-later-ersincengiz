#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

export SCRIPT_PATH=$(pwd)

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    
  #  make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    
  #  make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}
echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories

	mkdir -p rootfs/bin rootfs/dev rootfs/etc rootfs/home rootfs/lib rootfs/lib64 rootfs/proc rootfs/sbin rootfs/sys rootfs/tmp rootfs/usr rootfs/var
	
	mkdir -p rootfs/usr/bin rootfs/usr/lib rootfs/usr/sbin
	
	mkdir -p rootfs/var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    
     make distclean 
     
     make defconfig
     
else
    cd busybox
fi

# TODO: Make and install busybox
ROOTFS="${OUTDIR}/rootfs"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${ROOTFS} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ..



cd ${ROOTFS}

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"


# TODO: Add library dependencies to rootfs

SYSROOT=`${CROSS_COMPILE}gcc --print-sysroot`

cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 lib/
cp ${SYSROOT}/lib64/libm.so.6 lib64/
cp ${SYSROOT}/lib64/libresolv.so.2 lib64/
cp ${SYSROOT}/lib64/libc.so.6 lib64/


# TODO: Make device nodes


sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1


# TODO: Clean and build the writer utility

cd $SCRIPT_PATH
make clean
make CROSS_COMPILE=$CROSS_COMPILE writer

cd $ROOTFS

cp $SCRIPT_PATH/writer ${ROOTFS}/home/

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cd $ROOTFS


cp $SCRIPT_PATH/finder.sh ${ROOTFS}/home/
cp $SCRIPT_PATH/finder-test.sh ${ROOTFS}/home/

mkdir home/conf
cp $SCRIPT_PATH/conf/username.txt ${ROOTFS}/home/conf/
cp $SCRIPT_PATH/conf/assignment.txt ${ROOTFS}/home/conf/

cp $SCRIPT_PATH/autorun-qemu.sh ${ROOTFS}/home/

# TODO: Chown the root directory

cd ..

sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz

cd ${ROOTFS}

find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
