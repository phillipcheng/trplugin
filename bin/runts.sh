export LD_LIBRARY_PATH=/opt/fd/lib:/opt/ats/lib:/usr/local/lib
nohup bin/traffic_server -T"trafficreport" >tslog.txt 2>&1 &
