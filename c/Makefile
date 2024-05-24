CFLAGS = -Wall -Wextra -Werror -Wno-unused-parameter -O3 -fsanitize=address -fsanitize=undefined
CFLAGS_NO_ASAN = -Wall -Wextra -Werror -Wno-unused-parameter -O3
CC = gcc

tests:
	mkdir -p out
	${CC} ${CFLAGS} src/kc_string_test.c -o out/kc_string_test
	${CC} ${CFLAGS_NO_ASAN} src/kc_string_test.c -o out/kc_string_test_no_asan
	${CC} ${CFLAGS} src/hashmap/hashmap.c src/perms_test.c -o out/perms_test
	${CC} ${CFLAGS_NO_ASAN} src/hashmap/hashmap.c src/perms_test.c -o out/perms_test_no_asan

	# For every file in out, run it
	for file in out/*; do \
		$$file; \
	done

clean:
	rm -rf out