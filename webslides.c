#include <cairo-svg.h>
#include <cairo.h>
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "colorprint_header.h"

#define TAG_OK "[lg][+][/lg] "
#define TAG_INFO "[ly][*][/ly] "
#define TAG_FAIL "[bw][red]%d[/red][/bw] "

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

int convert(PopplerPage *page, const char *fname, char** comments) {
  cairo_surface_t *surface;
  cairo_t *img;
  double width, height;
  char* comm = calloc(1024, 1);

  poppler_page_get_size(page, &width, &height);
  surface = cairo_svg_surface_create(fname, width, height);
  img = cairo_create(surface);

  poppler_page_render_for_printing(page, img);
  
  GList* annot_list = poppler_page_get_annot_mapping(page);
  GList* s;
  for (s = annot_list; s != NULL; s = s->next) {
    PopplerAnnotMapping* m = (PopplerAnnotMapping *)s->data;
    int type = poppler_annot_get_annot_type(m->annot);
    if(type == 1) {
        char* cont = poppler_annot_get_contents(m->annot);
        strncat(comm, cont, 1024);
        strncat(comm, "\n", 1024);
        g_free(cont);
    } 
  }
  *comments = comm;
  poppler_page_free_annot_mapping(annot_list);  
  
  GList* link_list = poppler_page_get_link_mapping(page);
  for (s = link_list; s != NULL; s = s->next) {
    PopplerLinkMapping* m = (PopplerLinkMapping *)s->data; 
    PopplerAction* a = m->action;
    if(a->type == POPPLER_ACTION_LAUNCH) {
        PopplerActionLaunch* launch = (PopplerActionLaunch*)a;
        printf("\n\n%s\n", launch->file_name);        
        // TODO: handle embedded video
    }
  }
  poppler_page_free_link_mapping(link_list);
    
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
    printf_color(1, "Usage: webslides <in file.pdf>\n");
    return -1;
  }

  realpath(args[1], abspath);
  snprintf(fname_uri, PATH_MAX, "file://%s", abspath);

  pdffile = poppler_document_new_from_file(fname_uri, NULL, NULL);
  if (pdffile == NULL) {
    printf_color(1, TAG_FAIL "Could not open file '%s'\n", fname_uri);
    return -3;
  }

  int pages = poppler_document_get_n_pages(pdffile);
  printf_color(1, TAG_OK "Loaded %d slides\n", pages);
  
  FILE* template = fopen("index.html.template", "r");
  if(!template) {
      printf_color(1, TAG_FAIL "Could not open template file [m]index.html.template[/m]\n");
      return 1;
  }
  FILE* output = fopen("index.html", "w");
  if(!output) {
      printf_color(1, TAG_FAIL "Could not create output file [m]index.html[/m]\n");
      return 1;
  }
  size_t len = 0;
  char* line = NULL;
  
  printf_color(1, TAG_INFO "Converting slides...\n");
  progress_start(1, pages, NULL);  

  // copy template
  while(getline(&line, &len, template) != -1) {
    fprintf(output, "%s", line);
  }

  // annotations
  char* annotations[pages];
  memset(annotations, 0, sizeof(annotations));
  
  // add inline images
  fprintf(output, "var slide_img = [\n");
  for (int p = 0; p < pages; p++) {
    page = poppler_document_get_page(pdffile, p);
    convert(page, "slide.svg", &(annotations[p]));
    progress_update(1);
    char* b64 = encode("slide.svg");
    fprintf(output, "\"%s\",\n", b64);
    free(b64);
  }
  fprintf(output, "0];\n");
  
  // add annotations
  fprintf(output, "var slide_annot = [\n");
  for(int p = 0; p < pages; p++) {
      char enc[1024];
      base64encode(annotations[p], strlen(annotations[p]), enc, sizeof(enc));
      fprintf(output, "\"%s\",\n", enc);
      free(annotations[p]);
  }
  fprintf(output, "\"\"];</script>\n");
  unlink("slide.svg");
  
  printf_color(1, TAG_OK "Done!\n");
  
  return 0;
}
