tests:
	mkdir -p out
	gcc -fsanitize=address src/kc_string_test.c -o out/kc_string_test
	gcc -fsanitize=address src/hashmap/hashmap.c src/perms_test.c -o out/perms_test

	# For every file in out, run it
	for file in out/*; do \
		$$file; \
	done

clean:
	rm -rf out