#include "webslides.h"

#include <cairo-svg.h>
#include <cairo.h>
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli.h"
#include "colorprint_header.h"
#include "res.h"
#include "utils.h"

#if defined(__linux__) || defined(__linux) || defined(__unix__) || defined(LINUX) || defined(UNIX)
#define LINUX
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__CYGWIN__)
#define WINDOWS
#undef LINUX
#endif

#if defined(WINDOWS)
#include <shlwapi.h>
#include <windows.h>

void winsystem(const char* app, char* arg) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdInput = NULL;
  si.hStdError = NULL;
  si.hStdOutput = NULL;
  ZeroMemory(&pi, sizeof(pi));

  CreateProcess(app, arg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}
#endif

#define NO_SLIDES 0

// ---------------------------------------------------------------------------
static getopt_arg_t cli_options[] = {
    {"single", no_argument, NULL, 's', "Create a single file", NULL},
    {"presenter", no_argument, NULL, 'p', "Start in presenter mode", NULL},
    {"output", required_argument, NULL, 'o', "Output file name", "FILENAME"},
    {"disablenotes", no_argument, NULL, 'n', "Do not include notes", NULL},
    {"compress", required_argument, NULL, 'c', "Use an SVG compressor (e.g., svgcleaner)", "BINARY"},
    {"version", no_argument, NULL, 'v', "Show version", NULL},
    {"help", no_argument, NULL, 'h', "Show this help.", NULL},
    {NULL, 0, NULL, 0, NULL, NULL}};

// ---------------------------------------------------------------------------
void create_thumbnail(PopplerPage* page, const char* fname, int width, int height) {
    float scale = 72.0 * 4;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 150 / scale * width, 150 / scale * height);
    cairo_t* cr = cairo_create(surface);
    cairo_scale(cr, 150.0 / scale, 150.0 / scale);
    cairo_save(cr);
    poppler_page_render(page, cr);
    cairo_restore(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_write_to_png(surface, fname);

    cairo_surface_destroy(surface);
}

// ---------------------------------------------------------------------------
int convert(PopplerPage *page, const char *fname, SlideInfo *info) {
  cairo_surface_t *surface;
  cairo_t *img;
  double width, height;
  char *comm = calloc(1024, 1);
  char fname_prev[256];
  snprintf(fname_prev, 256, "%s.prev.png", fname);

  poppler_page_get_size(page, &width, &height);
  surface = cairo_svg_surface_create(fname, width, height);
  img = cairo_create(surface);

  poppler_page_render_for_printing(page, img);

  GList *annot_list = poppler_page_get_annot_mapping(page);
  GList *s;
  for (s = annot_list; s != NULL; s = s->next) {
    PopplerAnnotMapping *m = (PopplerAnnotMapping *)s->data;
    int type = poppler_annot_get_annot_type(m->annot);
    if (type == 1) {
      char *cont = poppler_annot_get_contents(m->annot);
      if (cont) {
        strncat(comm, cont, 1024);
        strncat(comm, "\n", 1024);
        g_free(cont);
      }
    }
  }
  info->annotations = comm;
  poppler_page_free_annot_mapping(annot_list);

  info->videos = NULL;
  GList *link_list = poppler_page_get_link_mapping(page);
  for (s = link_list; s != NULL; s = s->next) {
    PopplerLinkMapping *m = (PopplerLinkMapping *)s->data;
    PopplerAction *a = m->action;
    if (a->type == POPPLER_ACTION_LAUNCH) {
      PopplerActionLaunch *launch = (PopplerActionLaunch *)a;
      //         printf("\n\n%s\n", launch->file_name);
      info->videos = strdup(launch->file_name);
    }
  }
  poppler_page_free_link_mapping(link_list);

  cairo_show_page(img);
  create_thumbnail(page, fname_prev, width, height);
  
  cairo_destroy(img);
  cairo_surface_destroy(surface);
  return 0;
}

// ---------------------------------------------------------------------------
void progress_cb(int slide) { progress_update(1); }

// ---------------------------------------------------------------------------
void extract_slide(PopplerDocument *pdffile, int p, SlideInfo *info,
                   Options *options) {
  PopplerPage *page;
  char fname[64], fname_c[64], fname_p[128];
  sprintf(fname, "slide-%d.svg", p);
  sprintf(fname_p, "%s.prev.png", fname);
  page = poppler_document_get_page(pdffile, p);
  convert(page, fname, &(info[p]));
  if (options->nonotes) {
    free(info[p].annotations);
    info[p].annotations = "";
  }
  if (options->compress) {
    sprintf(fname_c, "slidec-%d.svg", p);
    char convert_cmd[256];
#if defined(WINDOWS)
    sprintf(convert_cmd, "\"%s\" %s %s", options->compress, fname, fname_c);
    winsystem(options->compress, convert_cmd);
#endif
#if defined(LINUX)
    sprintf(convert_cmd, "\"%s\" %s %s 2> /dev/null > /dev/null", options->compress, fname, fname_c);
    system(convert_cmd);
#endif
  } else {
    strcpy(fname_c, fname);
  }
  progress_update(1);
#if NO_SLIDES
  char *b64 = strdup(empty_img);
#else
  char *b64 = encode_file_base64(fname_c);
#endif
  info[p].slide = b64;
  info[p].thumb = encode_file_base64(fname_p);
  unlink(fname);
  unlink(fname_p);
  if (options->compress) {
      unlink(fname_c);
  }
}

// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  Options options = {.single = 0, .presenter = 0, .nonotes = 0, .name = NULL, .compress = NULL};
  PopplerDocument *pdffile;
  char abspath[PATH_MAX];
  char fname_uri[PATH_MAX + 32];
  
  if (argc <= 1) {
    show_usage(argv[0], cli_options);
    return 1;
  }
  if (parse_cli_options(&options, cli_options, argc, argv)) {
    return 1;
  }

  const char *input = argv[argc - 1];
  if (access(input, F_OK) == -1) {
    printf_color(1, TAG_FAIL "Could not open file '%s'\n", input);
    return 1;
  }

#if defined(LINUX)
  realpath(input, abspath);
  snprintf(fname_uri, sizeof(fname_uri), "file://%s", abspath);
#elif defined(WINDOWS)
  GetFullPathName(input, PATH_MAX, abspath, NULL);
  unsigned int urllen = PATH_MAX;
  fname_uri[0] = 0;
  UrlCreateFromPath(abspath, fname_uri, &urllen, 0);
#endif
  
  pdffile = poppler_document_new_from_file(fname_uri, NULL, NULL);
  if (pdffile == NULL) {
    printf_color(1, TAG_FAIL "Could not open file '%s'\n", fname_uri);
    return -3;
  }

  int pages = poppler_document_get_n_pages(pdffile);
  printf_color(1, TAG_OK "Loaded %d slides\n", pages);

  char fname[1024];
  snprintf(fname, sizeof(fname), "%s.html",
           options.name ? options.name : "index");
  FILE *output = fopen(fname, "w");
  if (!output) {
    printf_color(1,
                 TAG_FAIL "Could not create output file [m]index.html[/m]\n");
    return 1;
  }

  printf_color(1, TAG_INFO "Converting slides...\n");
  progress_start(1, (pages + 1) * 5 - 1, NULL);

  char *template = strdup((char*)index_html_template); //read_file("index.html.template");

  template = replace_string_first(
      template, "{{black.svg}}",
      encode_array_base64((char *)black_svg, black_svg_len));
  template = replace_string_first(
      template, "{{freeze.svg}}",
      encode_array_base64((char *)freeze_svg, freeze_svg_len));
  template =
      replace_string_first(template, "{{open.svg}}",
                           encode_array_base64((char *)open_svg, open_svg_len));

  SlideInfo info[pages + 1];
  memset(info, 0, sizeof(info));

  // create slide data
  for (int p = 0; p < pages; p++) {
    extract_slide(pdffile, p, info, &options);
  }
  info[pages].annotations = "";
  info[pages].slide = "";
  info[pages].videos = "";
  info[pages].thumb = "";

  char *thumb_data = encode_array(info, 3, pages + 1, 0, progress_cb);
  char *slide_data = encode_array(info, 2, pages + 1, 0, progress_cb);
  char *video_data = encode_array(info, 1, pages + 1, 1, progress_cb);
  char *annot_data = encode_array(info, 0, pages + 1, 1, progress_cb);

  if (options.single) {
    template = replace_string_first(template, "{{script}}",
                                    "<script type='text/javascript'>\n"
                                    "var slide_info = {"
                                    "'slides': {{slides}},\n"
                                    "'videos': {{videos}},\n"
                                    "'annotations': {{annotations}},\n"
                                    "'thumb': {{thumb}}\n"
                                    "};\n"
                                    "</script>");

    template = replace_string_first(template, "{{slides}}", slide_data);
    template = replace_string_first(template, "{{videos}}", video_data);
    template = replace_string_first(template, "{{annotations}}", annot_data);
    template = replace_string_first(template, "{{thumb}}", thumb_data);
  } else {
    char include[1024];
    snprintf(include, sizeof(include),
             "<script type='text/javascript' src='%s.js'></script>\n",
             options.name ? options.name : "slides");
    template = replace_string_first(template, "{{script}}", include);
    snprintf(include, sizeof(include), "%s.js",
             options.name ? options.name : "slides");
    FILE *f = fopen(include, "w");
    fprintf(f,
            "var slide_info = {'slides': %s,\n'videos': %s,\n'annotations': "
            "%s\n, 'thumb': %s\n};\n",
            slide_data, video_data, annot_data, thumb_data);
    fclose(f);
  }

  template = replace_string_first(template, "{{presenter}}",
                                  options.presenter ? "true" : "false");

  fwrite(template, strlen(template), 1, output);
  fclose(output);

  printf_color(1, TAG_OK "Done!\n");

  return 0;
}
