CC = clang
CFLAGS = -Wall -Wextra -Iinclude -fsanitize=address,undefined -MMD
LDFLAGS = -lz -lcrypto -fsanitize=address,undefined

bin/mygit: out/mygit.o \
	out/config_io.o out/hash_table.o out/index_io.o out/linked_list.o out/object_io.o out/ref_io.o out/transport.o out/util.o \
	out/add.o out/checkout.o out/commit.o out/fetch.o out/log.o out/push.o out/status.o
	$(CC) $(LDFLAGS) $^ -o $@

include $(wildcard out/*.d)

out/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f out/*.o out/*.d bin/mygit
