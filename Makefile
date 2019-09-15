all: webslides

webslides: webslides.c
	gcc webslides.c -o webslides `pkg-config --cflags --libs poppler-glib`
