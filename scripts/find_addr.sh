cd ..
export address=$@
addr2line -e out/oboskrnl -Cfpira 0x$address
objdump -d out/oboskrnl -C -M intel | grep --color -C 10 -n $address
cd scripts
