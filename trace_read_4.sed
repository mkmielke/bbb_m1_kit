:start
s/read(4, "\(.*\)",.*/\1/
t next
b
:next
s/\\r\\n$//
t star
N
s/\n//
b start
:star
s/^/> /
