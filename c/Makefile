CFLAGS = -DDEBUG_PRINTF=1 -g -Wall -Wextra -Werror -Wno-unused-parameter -O3 -fsanitize=address -fsanitize=undefined
CFLAGS_NO_ASAN = -g -Wall -Wextra -Werror -Wno-unused-parameter -O3
CC = gcc

tests:
	mkdir -p out
	${CC} ${CFLAGS} src/kc_string_test.c -o out/kc_string_test
	${CC} ${CFLAGS_NO_ASAN} src/kc_string_test.c -o out/kc_string_test_no_asan
	${CC} ${CFLAGS} src/hashmap/hashmap.c src/perms_test.c -o out/perms_test
	${CC} ${CFLAGS_NO_ASAN} src/hashmap/hashmap.c src/perms_test.c -o out/perms_test_no_asan

	./out/kc_string_test
	./out/perms_test

clean:
	rm -rf out
