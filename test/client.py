# coding=utf-8

from socket import *

HOST = 'localhost'
PORT = 12345

s = socket(AF_INET, SOCK_STREAM)
s.connect((HOST, PORT))

while True:
    msg = raw_input(">>")
    s.sendall(msg)
    if msg == 'exit':
        break;
    data = s.recv(1024)
    print 'recv:', repr(data)
    
s.close()