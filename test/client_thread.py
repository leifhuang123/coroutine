# coding=utf-8

from socket import *
from time import time, ctime,sleep
import sys
import threading

threads = []
HOST = 'localhost'
PORT = 12345

def conn_send():
    count = 0
    t = threading.current_thread()
    tid = t.ident
    res = 'count:' + str(count) + '__tid:' + str(tid)
    sock = socket(AF_INET, SOCK_STREAM)
    try:
        sock.connect((HOST, PORT))
        sock.send(res)
        szBuf = sock.recv(1024)
        print 'recv:', szBuf
        
        while szBuf != 'exit':
            count += 1
            res = 'count:' + str(count) + '__tid:' + str(tid)
            sock.send(res)
            szBuf = sock.recv(1024)
            print 'recv:', szBuf
            sleep(0.1)
            if count > 10:
                break
        
        sock.close()
    except IOError, e:
        print 'Error:', e
    
if __name__ == '__main__':
    for i in xrange(10):
        t = threading.Thread(target=conn_send, args=())
        threads.append(t)
        
    start = time()
    
    for t in threads:
        t.start()
    
    for t in threads:
        t.join()
        
    stop = time()
    print 'used time: %fs' % (stop-start)