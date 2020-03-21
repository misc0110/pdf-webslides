all: webslides

webslides: webslides.o cli.o utils.o
	gcc webslides.o cli.o utils.o -g -o webslides `pkg-config --libs poppler-glib`

webslides.o: webslides.c
	gcc webslides.c -c -g `pkg-config --cflags poppler-glib`
	
cli.o: cli.c
	gcc cli.c -c -g
	
utils.o: utils.c
	gcc utils.c -c -g
	
res.o: res.c
	gcc res.c -c -g

clean:
	rm -f *.o webslides
	
