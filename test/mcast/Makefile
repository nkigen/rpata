
default : svr cli

.PHONY: svr
svr:
	$(CC) -std=gnu11 -g -DSVR mcast.c -o svr
	
.PHONY: cli
cli:
	$(CC) -std=gnu11 -g -DCLI mcast.c -o cli

.PHONY: clean
clean:
	$(RM) svr cli
