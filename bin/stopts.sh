ps aux | grep -i traffic_server | awk {'print $2'} | xargs kill -9
