tests:
	mkdir -p out
	gcc -fsanitize=address -Wall -Wextra -Werror -Wno-unused-parameter src/kc_string_test.c -o out/kc_string_test
	gcc -fsanitize=address -Wall -Wextra -Werror -Wno-unused-parameter src/hashmap/hashmap.c src/perms_test.c -o out/perms_test
	gcc -Wall -Wextra -Werror -Wno-unused-parameter src/kc_string_test.c -o out/kc_string_test_no_asan
	gcc -Wall -Wextra -Werror -Wno-unused-parameter src/hashmap/hashmap.c src/perms_test.c -o out/perms_test_no_asan

	# For every file in out, run it
	for file in out/*; do \
		$$file; \
	done

clean:
	rm -rf out