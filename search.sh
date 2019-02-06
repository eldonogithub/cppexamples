f="
errno.h
fcntl.h
iostream
map
netdb.h
regex
sstream
stdio.h
stdlib.h
string
string.h
sys/epoll.h
sys/socket.h
sys/types.h
unistd.h
"

inc="
 /usr/lib/gcc/x86_64-linux-gnu/5/include
 /usr/local/include
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed
 /usr/include/x86_64-linux-gnu
 /usr/include
"

for i in $f; do 
  for j in $inc; do 
    bn=`basename $i`; 
    dn=`dirname $i`; 
    dn=${dn#.}; 
    dn=${dn:+/$dn}
    if [[ -f "$j$dn/$bn" ]]; then 
      grep -q $1 "$j$dn/$bn" && echo "#include <$i>" && break;
    fi; 
  done; 
done
