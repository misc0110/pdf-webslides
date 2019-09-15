#include <cairo-svg.h>
#include <cairo.h>
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int convert(PopplerPage *page, const char *fname) {
  cairo_surface_t *surface;
  cairo_t *img;
  double width, height;

  poppler_page_get_size(page, &width, &height);
  surface = cairo_svg_surface_create(fname, width, height);
  img = cairo_create(surface);

  poppler_page_render_for_printing(page, img);
  cairo_show_page(img);

  cairo_destroy(img);
  cairo_surface_destroy(surface);

  return 0;
}

int main(int argc, char *args[]) {
  PopplerDocument *pdffile;
  PopplerPage *page;
  char abspath[PATH_MAX];
  char fname_uri[PATH_MAX];
  char svg_fname[PATH_MAX];

  if (argc != 2) {
    printf("Usage: pdf2svg <in file.pdf>\n");
    return -2;
  }

  realpath(args[1], abspath);
  snprintf(fname_uri, PATH_MAX, "file://%s", abspath);

  pdffile = poppler_document_new_from_file(fname_uri, NULL, NULL);
  if (pdffile == NULL) {
    fprintf(stderr, "Could not open file '%s'\n", fname_uri);
    return -3;
  }

  int pages = poppler_document_get_n_pages(pdffile);

  for (int p = 0; p < pages; p++) {

    page = poppler_document_get_page(pdffile, p);
    snprintf(svg_fname, PATH_MAX, "slide%03d.svg", p + 1);
    convert(page, svg_fname);
  }
  return 0;
}
