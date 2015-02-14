# xip
linux c下基于纯真ip数据库开发的ip查询http形式的server


#先安装libevent2     
yum install -y libevent libevent-devel        


#安装      
gcc -Wall -c libqqwry/qqwry.c         
gcc -Wall -c xip.c   -levent -L /usr/local/libevent2/lib/ -I /usr/local/libevent2/include/          
gcc -Wall -o xip qqwry.o cJSON.o xip.o   -levent -L /usr/local/libevent2/lib/ -I /usr/local/libevent2/include/ -lm -liconv        


#运行            
./http -l 192.168.8.21 -p 8020 -d       


#使用     
http://192.168.8.21:8020/?type=getip&ip=58.50.20.14   


#结果    
[qidasheng@master ~]$ curl "http://192.168.8.21:8020/?type=getip&ip=58.50.20.14"
{
	"ip":	"58.50.20.14",
	"address":	"湖北省荆州市沙市区  大学旁洪苑一区珍珍网吧",
	"info":	{
		"code":	200,
		"province":	"湖北省",
		"city":	"荆州市沙市区",
		"desc":	"大学旁洪苑一区珍珍网吧",
		"time":	"0.00006600"
	}
}
