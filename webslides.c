#include <cairo-svg.h>
#include <cairo.h>
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


int base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize)
{
   const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   const uint8_t *data = (const uint8_t *)data_buf;
   size_t resultIndex = 0;
   size_t x;
   uint32_t n = 0;
   int padCount = dataLength % 3;
   uint8_t n0, n1, n2, n3;

   for (x = 0; x < dataLength; x += 3) 
   {
      n = ((uint32_t)data[x]) << 16;
      
      if((x+1) < dataLength)
         n += ((uint32_t)data[x+1]) << 8;
      
      if((x+2) < dataLength)
         n += data[x+2];

      n0 = (uint8_t)(n >> 18) & 63;
      n1 = (uint8_t)(n >> 12) & 63;
      n2 = (uint8_t)(n >> 6) & 63;
      n3 = (uint8_t)n & 63;
            
      if(resultIndex >= resultSize) return 1;
      result[resultIndex++] = base64chars[n0];
      if(resultIndex >= resultSize) return 1;
      result[resultIndex++] = base64chars[n1];

      if((x+1) < dataLength)
      {
         if(resultIndex >= resultSize) return 1;
         result[resultIndex++] = base64chars[n2];
      }
      if((x+2) < dataLength)
      {
         if(resultIndex >= resultSize) return 1;
         result[resultIndex++] = base64chars[n3];
      }
   }
   if (padCount > 0) 
   { 
      for (; padCount < 3; padCount++) 
      { 
         if(resultIndex >= resultSize) return 1;
         result[resultIndex++] = '=';
      } 
   }
   if(resultIndex >= resultSize) return 1;
   result[resultIndex] = 0;
   return 0;
}

char* encode(char* fname) {
    FILE* f = fopen(fname, "rb");
    fseek(f, 0, SEEK_END);
    size_t s = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* in = malloc(s);
    fread(in, s, 1, f);
    fclose(f);
    char* out = malloc(s * 2 + 8);
    base64encode(in, s, out, s * 2 + 8);
    free(in);
    return out;
}

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

  if (argc != 2) {
    printf("Usage: webslides <in file.pdf>\n");
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
  
  FILE* template = fopen("index.html.template", "r");
  FILE* output = fopen("index.html", "w");
  size_t len = 0;
  char* line = NULL;
  // copy template
  while(getline(&line, &len, template) != -1) {
    fprintf(output, "%s", line);
  }

  // add inline images
  fprintf(output, "slide_img = [\n");
  for (int p = 0; p < pages; p++) {
    page = poppler_document_get_page(pdffile, p);
    convert(page, "slide.svg");
    char* b64 = encode("slide.svg");
    fprintf(output, "\"%s\",\n", b64);
    free(b64);
  }
  fprintf(output, "0];</script>\n");
  unlink("slide.svg");
  return 0;
}
