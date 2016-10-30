src=main.c
opt= -Wall -Werror -g
.PHONY: all clean

all: $(src)
	gcc $(src) $(opt) -o arith

clean: 
	rm -rf *.o  arith > /dev/null 2>&1
