#ifndef TSMTERM_MODULE_H
#define TSMTERM_MODULE_H

#include "emacs-module.h"
#include <inttypes.h>
#include <stdbool.h>
#include <vterm.h>

int plugin_is_GPL_compatible;
#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif
#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

typedef struct  {
  emacs_env *env ;
  struct tsm_screen *screen;
  struct tsm_vte *vte;
  unsigned int cell_width;
  unsigned int cell_height;
  unsigned int width;
  unsigned int height;
  unsigned int columns;
  unsigned int rows;

  unsigned int adjust_size : 1;
  unsigned int initialized : 1;
  unsigned int exited : 1;

  char outputbuf[0x1fff];
  size_t outputbuf_len;

} Term;

// Faces
emacs_value Qterm;
emacs_value Qterm_color_black;
emacs_value Qterm_color_red;
emacs_value Qterm_color_green;
emacs_value Qterm_color_yellow;
emacs_value Qterm_color_blue;
emacs_value Qterm_color_magenta;
emacs_value Qterm_color_cyan;
emacs_value Qterm_color_white;

static bool is_key(unsigned char *key, size_t len, char *key_description);


static void term_redraw(Term *term, emacs_env *env);
/* static void term_setup_colors(Term *term, emacs_env *env); */
static void term_flush_output(Term *term, emacs_env *env);
static void term_process_key(Term *term, unsigned char *key, size_t len,
                             VTermModifier modifier);
static void term_finalize(void *object);

static emacs_value Fvterm_new(emacs_env *env, ptrdiff_t nargs,
                              emacs_value args[], void *data);
static emacs_value Fvterm_update(emacs_env *env, ptrdiff_t nargs,
                                 emacs_value args[], void *data);
int emacs_module_init(struct emacs_runtime *ert);

#endif /* TSMTERM_MODULE_H */
