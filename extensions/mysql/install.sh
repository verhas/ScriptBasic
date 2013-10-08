cc -c -o mysqlinterf.o mysqlinterf.c
ld -shared -o mysql.so mysqlinterf.o -lc -lmysqlclient
mv *.so /etc/scriba/modules

