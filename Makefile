# WARNING ABOUT TESTS
# -------------------
#
# Feel free to modify this makefile for your own purposes. However, tests will
# require that when submitting:
#  - `make mygit` builds the binary
#  - All sanitizers MUST be off for the above command

CC = clang
CFLAGS = -Wall -Wextra -Iinclude -MMD #-fsanitize=address,undefined
LDFLAGS = -lz -lcrypto #-fsanitize=address,undefined

bin/mygit: out/mygit.o \
	out/config_io.o out/hash_table.o out/index_io.o out/linked_list.o out/object_io.o out/ref_io.o out/transport.o out/util.o \
	out/add.o out/checkout.o out/commit.o out/fetch.o out/log.o out/push.o out/status.o
	$(CC) $(LDFLAGS) $^ -o $@

include $(wildcard out/*.d)

out/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f out/*.o out/*.d bin/mygit
