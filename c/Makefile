CFLAGS = -DDEBUG_PRINTF=1 -g -Wall -Wextra -Werror -Wno-unused-parameter -O3 -fsanitize=address -fsanitize=undefined
CFLAGS_NO_ASAN = -g -Wall -Wextra -Werror -Wno-unused-parameter -O3
CFLAGS_C99 = -g -Wall -Wextra -Werror -Wno-unused-parameter -O3 -DC99 -std=c99 -fsanitize=address -fsanitize=undefined
CFLAGS_C99_NO_ASAN = -g -Wall -Wextra -Werror -Wno-unused-parameter -O3 -DC99 -std=c99
CC = gcc

lib_dynamic_so:
	${CC} ${CFLAGS} --shared -fPIC src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat.so
	${CC} ${CFLAGS_NO_ASAN} --shared -fPIC src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat_no_asan.so
	${CC} ${CFLAGS_C99} --shared -fPIC src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat_c99.so
	${CC} ${CFLAGS_C99_NO_ASAN} --shared -fPIC src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat_c99_no_asan.so

lib_dynamic_dylib:
	${CC} ${CFLAGS} -dynamiclib src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat.dylib
	${CC} ${CFLAGS_NO_ASAN} -dynamiclib src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat_no_asan.dylib
	${CC} ${CFLAGS_C99} -dynamiclib src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat_c99.dylib
	${CC} ${CFLAGS_C99_NO_ASAN} -dynamiclib src/hashmap.c src/perms.c src/kc_string.c src/prelude.c -o out/libkittycat_c99_no_asan.dylib

lib_static:
	${CC} ${CFLAGS} -c src/hashmap.c src/perms.c src/kc_string.c src/prelude.c
	ar rcs out/libkittycat.a hashmap.o perms.o kc_string.o prelude.o
	${CC} ${CFLAGS_NO_ASAN} -c src/hashmap.c src/perms.c src/kc_string.c src/prelude.c
	ar rcs out/libkittycat_no_asan.a hashmap.o perms.o kc_string.o prelude.o
	${CC} ${CFLAGS_C99} -c src/hashmap.c src/perms.c src/kc_string.c src/prelude.c
	ar rcs out/libkittycat_c99.a hashmap.o perms.o kc_string.o prelude.o
	${CC} ${CFLAGS_C99_NO_ASAN} -c src/hashmap.c src/perms.c src/kc_string.c src/prelude.c
	ar rcs out/libkittycat_c99_no_asan.a hashmap.o perms.o kc_string.o prelude.o

tests:
	mkdir -p out
	${CC} ${CFLAGS} src/kc_string_test.c src/prelude.c src/hashmap.c -o out/kc_string_test
	${CC} ${CFLAGS_NO_ASAN} src/kc_string_test.c src/prelude.c src/hashmap.c -o out/kc_string_test_no_asan
	${CC} ${CFLAGS_C99} src/kc_string_test.c src/prelude.c src/hashmap.c -o out/kc_string_test_c99
	${CC} ${CFLAGS_C99_NO_ASAN} src/kc_string_test.c src/prelude.c src/hashmap.c -o out/kc_string_test_c99_no_asan
	${CC} ${CFLAGS} src/hashmap.c src/perms_test.c src/kc_string.c src/prelude.c -o out/perms_test
	${CC} ${CFLAGS_NO_ASAN} src/hashmap.c src/perms_test.c src/kc_string.c src/prelude.c -o out/perms_test_no_asan
	${CC} ${CFLAGS_C99} src/hashmap.c src/perms_test.c src/kc_string.c src/prelude.c -o out/perms_test_c99
	${CC} ${CFLAGS_C99_NO_ASAN} src/hashmap.c src/perms_test.c src/kc_string.c src/prelude.c -o out/perms_test_c99_no_asan

	./out/kc_string_test
	./out/kc_string_test_no_asan
	./out/kc_string_test_c99
	./out/kc_string_test_c99_no_asan
	./out/perms_test
	./out/perms_test_no_asan
	./out/perms_test_c99
	./out/perms_test_c99_no_asan

clean:
	rm -rf out
