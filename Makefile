CFLAGS += -g -Wall
LDFLAGS += 

all: webslides

webslides: webslides.o cli.o utils.o res.o
	gcc webslides.o cli.o utils.o res.o $(LDFLAGS) -o webslides `pkg-config --libs poppler-glib`

webslides.o: webslides.c
	gcc webslides.c -c $(CFLAGS) `pkg-config --cflags poppler-glib`
	
cli.o: cli.c
	gcc cli.c -c $(CFLAGS)
	
utils.o: utils.c
	gcc utils.c -c $(CFLAGS)
	
res.o: res.c freeze.svg 
	xxd -i freeze.svg > res_gen.c
	xxd -i black.svg >> res_gen.c
	xxd -i open.svg >> res_gen.c
	gcc res.c -c $(CFLAGS)

clean:
	rm -f *.o webslides
	
