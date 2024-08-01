rm ./1
rmdir ./mount
mkdir ./mount
gcc -w 1.c -o 1 -lfuse
./1 -f mount/


