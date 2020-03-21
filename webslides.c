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
#include "cli.h"
#include "res.h"
#include "utils.h"

#define NO_SLIDES 0

#define TAG_OK "[lg][+][/lg] "
#define TAG_INFO "[ly][*][/ly] "
#define TAG_FAIL "[bw][red][-][/red][/bw] "

static getopt_arg_t cli_options[] =
        {
                {"single",    no_argument,       NULL, 's', "Create a single file", NULL},
                {"presenter", no_argument,       NULL, 'p', "Include presenter", NULL},
                {"help",      no_argument,       NULL, 'h', "Show this help.",       NULL},
                {NULL, 0,                        NULL, 0, NULL,                      NULL}
        };



int convert(PopplerPage *page, const char *fname, char** comments, char** videos) {
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
        if(cont) {
            strncat(comm, cont, 1024);
            strncat(comm, "\n", 1024);
            g_free(cont);
        }
    } 
  }
  *comments = comm;
  poppler_page_free_annot_mapping(annot_list);  
  
  *videos = NULL;
  GList* link_list = poppler_page_get_link_mapping(page);
  for (s = link_list; s != NULL; s = s->next) {
    PopplerLinkMapping* m = (PopplerLinkMapping *)s->data; 
    PopplerAction* a = m->action;
    if(a->type == POPPLER_ACTION_LAUNCH) {
        PopplerActionLaunch* launch = (PopplerActionLaunch*)a;
//         printf("\n\n%s\n", launch->file_name);    
        *videos = strdup(launch->file_name);
    }
  }
  poppler_page_free_link_mapping(link_list);
    
  cairo_show_page(img);

  cairo_destroy(img);
  cairo_surface_destroy(surface);
  return 0;
}

int main(int argc, char *argv[]) {
  Options options;
  PopplerDocument *pdffile;
  PopplerPage *page;
  char abspath[PATH_MAX];
  char fname_uri[PATH_MAX];

  if(argc <= 1) {
    show_usage(argv[0], &options);
    return 1;
  }
  if(parse_cli_options(&options, cli_options, argc, argv)) {
    return 1;
  }
  
  const char* input = argv[argc - 1];
  if(access(input, F_OK ) == -1) {
    printf_color(1, TAG_FAIL "Could not open file '%s'\n", input);
    return 1;
  }

  realpath(input, abspath);
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
  
  // videos
  char* videos[pages];
  memset(videos, 0, sizeof(pages));
  
  // add inline images
  fprintf(output, "var slide_img = [\n");
  for (int p = 0; p < pages; p++) {
    page = poppler_document_get_page(pdffile, p);
    convert(page, "slide.svg", &(annotations[p]), &(videos[p]));
    progress_update(1);
#if NO_SLIDES
    char* b64 = empty_img;
#else
    char* b64 = encode_file_base64("slide.svg");
#endif
    fprintf(output, "\"%s\",\n", b64);
#if !NO_SLIDES
    free(b64);
#endif
  }
  fprintf(output, "0];\n");
  
  // add videos
  fprintf(output, "var slide_video = [\n");
  for(int p = 0; p < pages; p++) {
      char enc[1024];
      if(!videos[p]) {
        strcpy(enc, "");
      } else {
        base64encode(videos[p], strlen(videos[p]), enc, sizeof(enc));
      }
      fprintf(output, "\"%s\",\n", enc);
      free(videos[p]);
  }
  fprintf(output, "\"\"];\n");
  
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
