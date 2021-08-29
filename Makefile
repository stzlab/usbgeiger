SRC = usbgeiger.c
EXE = usbgeiger

$(EXE): $(SRC)
	gcc -Wall -O -o $@ $^ -lhidapi-hidraw

clean: 
	rm -f $(EXE)
