export LD_LIBRARY_PATH+="/usr/local/lib:../env/jsoncpp/libs/linux-gcc-4.4.7/:../env/hiredis/lib/:"

if [ "$1"x = "clr"x ]; then
	rm -f *.log
fi

./server 

