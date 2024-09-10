all:
	gcc naming_server/naming_server.c -o naming_server/nm
	gcc storage_server/storage_server.c -o storage_server/ss
	gcc storage_server/replica_server.c -o storage_server/replica1/rp
	gcc storage_server/replica_server.c -o naming_server/replica2/rp
	gcc clients/common_client.c -o clients/cc

clean:
	rm -f *.o *.exe *.tar.gz
	rm -f naming_server/nm storage_server/ss storage_server/replica1/rp naming_server/replica2/rp clients/cc