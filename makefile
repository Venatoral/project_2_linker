prog: *.cpp inc/*.hpp inc/*.h
	clang++ *.cpp -o $@

.PHONY:clean

clean:
	rm ./prog