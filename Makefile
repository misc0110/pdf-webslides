all: webslides

webslides: webslides.c
	gcc webslides.c -g -o webslides `pkg-config --cflags --libs poppler-glib`
