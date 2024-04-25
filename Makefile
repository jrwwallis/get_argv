CFLAGS += -Wall -Werror
get_argv_test: get_argv.o get_argv_test.o

clean:
	rm -f *.o get_argv_test
