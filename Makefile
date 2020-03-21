CFLAGS += -g -Wall 
LDFLAGS += 

all: webslides

webslides: webslides.o cli.o utils.o
	gcc webslides.o cli.o utils.o $(LDFLAGS) -o webslides `pkg-config --libs poppler-glib`

webslides.o: webslides.c
	gcc webslides.c -c $(CFLAGS) `pkg-config --cflags poppler-glib`
	
cli.o: cli.c
	gcc cli.c -c $(CFLAGS)
	
utils.o: utils.c
	gcc utils.c -c $(CFLAGS)
	
res.o: res.c
	gcc res.c -c $(CFLAGS)

clean:
	rm -f *.o webslides
	
