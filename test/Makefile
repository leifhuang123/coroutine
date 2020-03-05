target: test test_poll

test: test_coroutine.c socket.c ../src/coroutine.c
	gcc -g -o $@ $^
    
test_poll: test_co_poll.c socket.c ../src/coroutine.c
	gcc -g -o $@ $^
    
clean:
	rm test test_poll
