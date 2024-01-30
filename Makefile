build:
	g++ -Wall -std=c++17 -O2 -g 1brc.cpp -o 1brc

run:
	# Remember to plug in your laptop!
	./1brc


test-approximate:
    # cheap test: if the diff is <10 lines long, the results are probably good enough.
	# Do this approximate testing because I want to ignore small amounts of output etc.
	diffstr=$$(./1brc | sort | diff result_sorted_py.txt -); \
	echo "$${diffstr}"; \
	test $$(echo "$$diffstr" | wc -l) -le 10;

profile:
	g++ -Wall -std=c++17 -O1 -g 1brc.cpp -o 1brc  # NOTE: fow now, use -O1 to make it easier to read (!?)
	rm callgrind.out.* ; valgrind --tool=callgrind ./1brc ; kcachegrind
