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
#include "webslides.h"

#define NO_SLIDES 0

static getopt_arg_t cli_options[] =
{
        {"single",       no_argument,       NULL, 's', "Create a single file", NULL},
        {"presenter",    no_argument,       NULL, 'p', "Start in presenter mode", NULL},
        {"output",       required_argument, NULL, 'o', "Output file name", "FILENAME"}, 
        {"disablenotes", no_argument,       NULL, 'n', "Do not include notes", NULL},
        {"help",         no_argument,       NULL, 'h', "Show this help.",       NULL},
        {NULL, 0,                        NULL, 0, NULL,                      NULL}
};


int convert(PopplerPage *page, const char *fname, SlideInfo* info) {
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
  info->annotations = comm;
  poppler_page_free_annot_mapping(annot_list);  
  
  info->videos = NULL;
  GList* link_list = poppler_page_get_link_mapping(page);
  for (s = link_list; s != NULL; s = s->next) {
    PopplerLinkMapping* m = (PopplerLinkMapping *)s->data; 
    PopplerAction* a = m->action;
    if(a->type == POPPLER_ACTION_LAUNCH) {
        PopplerActionLaunch* launch = (PopplerActionLaunch*)a;
//         printf("\n\n%s\n", launch->file_name);    
        info->videos = strdup(launch->file_name);
    }
  }
  poppler_page_free_link_mapping(link_list);
    
  cairo_show_page(img);

  cairo_destroy(img);
  cairo_surface_destroy(surface);
  return 0;
}


void progress_cb(int slide) {
    progress_update(1);
}

void extract_slide(PopplerDocument* pdffile, int p, SlideInfo* info, Options* options) {
    PopplerPage *page;
    char fname[64];
    sprintf(fname, "slide-%d.svg", p);
    page = poppler_document_get_page(pdffile, p);
    convert(page, fname, &(info[p])); 
    if(options->nonotes) {
        free(info[p].annotations);
        info[p].annotations = "";
    }
    progress_update(1);
#if NO_SLIDES
    char* b64 = strdup(empty_img);
#else
    char* b64 = encode_file_base64(fname);
#endif
    info[p].slide = b64;
    unlink(fname);
}


int main(int argc, char *argv[]) {
  Options options = {.single = 0, .presenter = 0, .nonotes = 0, .name = NULL};
  PopplerDocument *pdffile;
  char abspath[PATH_MAX];
  char fname_uri[PATH_MAX + 32];

  if(argc <= 1) {
    show_usage(argv[0], cli_options);
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
  snprintf(fname_uri, sizeof(fname_uri), "file://%s", abspath);

  pdffile = poppler_document_new_from_file(fname_uri, NULL, NULL);
  if (pdffile == NULL) {
    printf_color(1, TAG_FAIL "Could not open file '%s'\n", fname_uri);
    return -3;
  }

  int pages = poppler_document_get_n_pages(pdffile);
  printf_color(1, TAG_OK "Loaded %d slides\n", pages);
  
  char fname[1024];
  snprintf(fname, sizeof(fname), "%s.html", options.name ? options.name : "index");
  FILE* output = fopen(fname, "w");
  if(!output) {
      printf_color(1, TAG_FAIL "Could not create output file [m]index.html[/m]\n");
      return 1;
  }
  
  printf_color(1, TAG_INFO "Converting slides...\n");
  progress_start(1, pages * 4, NULL);  

  char* template = read_file("index.html.template");
  if(!template) {
    printf_color(1, TAG_FAIL "Could not open file [m]%s[/m]\n", "index.html.template");
    return 1;
  }
  
  template = replace_string_first(template, "{{black.svg}}", encode_file_base64("black.svg"));
  template = replace_string_first(template, "{{freeze.svg}}", encode_file_base64("freeze.svg"));
  template = replace_string_first(template, "{{open.svg}}", encode_file_base64("open.svg"));

  SlideInfo info[pages];
  
  // create slide data
  for (int p = 0; p < pages; p++) {
    extract_slide(pdffile, p, info, &options);
  }
  
  char* slide_data = encode_array(info, 2, pages, 0, progress_cb);
  char* video_data = encode_array(info, 1, pages, 1, progress_cb);
  char* annot_data = encode_array(info, 0, pages, 1, progress_cb);
  
  if(options.single) {
    template = replace_string_first(template, "{{script}}", "<script type='text/javascript'>\n"
        "var slide_info = {"
        "'slides': {{slides}},\n"
        "'videos': {{videos}},\n"
        "'annotations': {{annotations}}\n"
        "};\n"
        "</script>");
    
    template = replace_string_first(template, "{{slides}}", slide_data);
    template = replace_string_first(template, "{{videos}}", video_data);
    template = replace_string_first(template, "{{annotations}}", annot_data);
  } else {
    char include[1024];
    snprintf(include, sizeof(include), "<script type='text/javascript' src='%s.js'></script>\n", options.name ? options.name : "slides");
    template = replace_string_first(template, "{{script}}", include);
    snprintf(include, sizeof(include), "%s.js", options.name ? options.name : "slides");
    FILE* f = fopen(include, "w");
    fprintf(f, "var slide_info = {'slides': %s,\n'videos': %s,\n'annotations': %s\n};\n", slide_data, video_data, annot_data);
    fclose(f);
  }
  
  template = replace_string_first(template, "{{presenter}}", options.presenter ? "true" : "false");
  
  fwrite(template, strlen(template), 1, output);
  fclose(output);
  
  printf_color(1, TAG_OK "Done!\n");
  
  return 0;
}
