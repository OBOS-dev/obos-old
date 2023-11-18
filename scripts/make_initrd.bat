@echo off
cd ../isodir/obos/initrd
tar --format ustar -cf ..\initrd.tar *
cd ../../../scripts