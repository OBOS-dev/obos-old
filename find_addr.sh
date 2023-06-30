export address=$@
addr2line -e out/oboskrnl -f -p -i -r -a 0x$address
 objdump -d out/oboskrnl  -M intel | grep --color -C 10 -n $address
