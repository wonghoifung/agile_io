pid=`lsof -i:1080|awk '{print $2}'|awk 'NR==2'`

if [ ! -n "${pid}" ]; then
  echo "no server 1080 found"
  exit 1
fi

kill -9 ${pid}


