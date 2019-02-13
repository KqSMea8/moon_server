# moon_server
a simple server framework, used to consturct a service quickly, support redis protocol

## 1. how to compile ?

  cd src && make

## 2. how to start server ?

  ./moonserver -p [server_port] -D [server_directory] -H [meta_ip] -P [meta_port] 

  deamon mode
  
  ./moonserver -p [server_port] -D [server_directory] -H [meta_ip] -P [meta_port] -d

## 3. how to access server?

  first install redis client programe， then use redis client access server

  redis-cli -p [xxxx] info
