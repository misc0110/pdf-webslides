CFLAGS += -g -Wall
LDFLAGS +=  
CC ?= gcc

all: webslides

webslides: webslides.o cli.o utils.o res.o
	$(CC) webslides.o cli.o utils.o res.o $(LDFLAGS) -o webslides `pkg-config --libs poppler-glib`

webslides.o: webslides.c
	$(CC) webslides.c -c $(CFLAGS) `pkg-config --cflags --static poppler-glib`
	
cli.o: cli.c
	$(CC) cli.c -c $(CFLAGS)
	
utils.o: utils.c
	$(CC) utils.c -c $(CFLAGS)
	
res.o: resconv res.c freeze.svg 
	./resconv freeze.svg > res_gen.c
	./resconv black.svg >> res_gen.c
	./resconv open.svg >> res_gen.c
	./resconv index.html.template >> res_gen.c
	$(CC) res.c -c $(CFLAGS)

resconv: resconv.c
	$(CC) resconv.c -o resconv

deb: webslides
	fakeroot -u ./makedeb.sh
	
clean:
	rm -f *.o *.deb webslides resconv	
