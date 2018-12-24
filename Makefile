CC=gcc -Wall -std=gnu99
OUT=main
SRC=$(OUT).c

$(OUT): $(SRC)
	$(CC) $< -o $@

clean:
	rm -rf $(OUT)
