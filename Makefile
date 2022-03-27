CC = clang
CFLAGS = -Wall -Wextra -fsanitize=address,undefined -MMD
LDFLAGS = -lz -lcrypto -fsanitize=address,undefined

mygit: mygit.o \
	config_io.o hash_table.o index_io.o linked_list.o object_io.o ref_io.o transport.o util.o \
	add.o checkout.o commit.o fetch.o log.o push.o status.o

include $(wildcard *.d)

clean:
	rm -f *.o *.d mygit
