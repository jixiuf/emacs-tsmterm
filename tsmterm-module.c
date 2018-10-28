#include "tsmterm-module.h"
#include "elisp.h"
#include "utf8.h"
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <libtsm.h>




static void term_redraw(Term *term, emacs_env *env) {
}




static bool is_key(unsigned char *key, size_t len, char *key_description) {
  return (len == strlen(key_description) &&
          memcmp(key, key_description, len) == 0);
}



static void term_flush_output(Term *term, emacs_env *env) {
}

static void term_process_key(Term *term, unsigned char *key, size_t len,
                             VTermModifier modifier) {
}


static void term_finalize(void *object) {
  Term *term = (Term *)object;
  /* tsmterm_free(term->vt); */
  free(term);
}

static emacs_value Ftsmterm_new(emacs_env *env, ptrdiff_t nargs,
                              emacs_value args[], void *data) {
  Term *term = malloc(sizeof(Term));
  return env->make_user_ptr(env, term_finalize, term);
}

static emacs_value Ftsmterm_update(emacs_env *env, ptrdiff_t nargs,
                                 emacs_value args[], void *data) {
  Term *term = env->get_user_ptr(env, args[0]);


  // Flush output
  term_flush_output(term, env);

  term_redraw(term, env);

  return env->make_integer(env, 0);
}

static emacs_value Ftsmterm_write_input(emacs_env *env, ptrdiff_t nargs,
                                      emacs_value args[], void *data) {
  Term *term = env->get_user_ptr(env, args[0]);
  ptrdiff_t len = string_bytes(env, args[1]);
  char bytes[len];

  env->copy_string_contents(env, args[1], bytes, &len);

  return env->make_integer(env, 0);
}
static emacs_value Ftsmterm_set_size(emacs_env *env, ptrdiff_t nargs,
                                   emacs_value args[], void *data) {
  Term *term = env->get_user_ptr(env, args[0]);
  int rows = env->extract_integer(env, args[1]);
  int cols = env->extract_integer(env, args[2]);


  return Qnil;
}

int emacs_module_init(struct emacs_runtime *ert) {
  emacs_env *env = ert->get_environment(ert);

  // Symbols;
  Qt = env->make_global_ref(env, env->intern(env, "t"));
  Qnil = env->make_global_ref(env, env->intern(env, "nil"));
  Qnormal = env->make_global_ref(env, env->intern(env, "normal"));
  Qbold = env->make_global_ref(env, env->intern(env, "bold"));
  Qitalic = env->make_global_ref(env, env->intern(env, "italic"));
  Qforeground = env->make_global_ref(env, env->intern(env, ":foreground"));
  Qbackground = env->make_global_ref(env, env->intern(env, ":background"));
  Qweight = env->make_global_ref(env, env->intern(env, ":weight"));
  Qunderline = env->make_global_ref(env, env->intern(env, ":underline"));
  Qslant = env->make_global_ref(env, env->intern(env, ":slant"));
  Qreverse = env->make_global_ref(env, env->intern(env, ":inverse-video"));
  Qstrike = env->make_global_ref(env, env->intern(env, ":strike-through"));
  Qface = env->make_global_ref(env, env->intern(env, "font-lock-face"));
  Qcursor_type = env->make_global_ref(env, env->intern(env, "cursor-type"));

  // Functions
  Flength = env->make_global_ref(env, env->intern(env, "length"));
  Flist = env->make_global_ref(env, env->intern(env, "list"));
  Ferase_buffer = env->make_global_ref(env, env->intern(env, "erase-buffer"));
  Finsert = env->make_global_ref(env, env->intern(env, "insert"));
  Fgoto_char = env->make_global_ref(env, env->intern(env, "goto-char"));
  Fput_text_property =
      env->make_global_ref(env, env->intern(env, "put-text-property"));
  Fset = env->make_global_ref(env, env->intern(env, "set"));
  Ftsmterm_face_color_hex =
      env->make_global_ref(env, env->intern(env, "tsmterm--face-color-hex"));
  Ftsmterm_flush_output =
      env->make_global_ref(env, env->intern(env, "tsmterm--flush-output"));
  Fforward_line = env->make_global_ref(env, env->intern(env, "forward-line"));
  Fgoto_line = env->make_global_ref(env, env->intern(env, "tsmterm--goto-line"));
  Fbuffer_line_number =
      env->make_global_ref(env, env->intern(env, "tsmterm--buffer-line-num"));
  Fdelete_lines =
      env->make_global_ref(env, env->intern(env, "tsmterm--delete-lines"));
  Frecenter = env->make_global_ref(env, env->intern(env, "tsmterm--recenter"));
  Fforward_char =
      env->make_global_ref(env, env->intern(env, "tsmterm--forward-char"));
  Fblink_cursor_mode =
      env->make_global_ref(env, env->intern(env, "blink-cursor-mode"));

  // Faces
  Qterm = env->make_global_ref(env, env->intern(env, "tsmterm"));
  Qterm_color_black =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-black"));
  Qterm_color_red =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-red"));
  Qterm_color_green =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-green"));
  Qterm_color_yellow =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-yellow"));
  Qterm_color_blue =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-blue"));
  Qterm_color_magenta =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-magenta"));
  Qterm_color_cyan =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-cyan"));
  Qterm_color_white =
      env->make_global_ref(env, env->intern(env, "tsmterm-color-white"));

  // Exported functions
  emacs_value fun;
  fun =
      env->make_function(env, 3, 3, Ftsmterm_new, "Allocates a new tsmterm.", NULL);
  bind_function(env, "tsmterm--new", fun);

  fun = env->make_function(env, 1, 5, Ftsmterm_update,
                           "Process io and update the screen.", NULL);
  bind_function(env, "tsmterm--update", fun);

  fun = env->make_function(env, 2, 2, Ftsmterm_write_input,
                           "Write input to tsmterm.", NULL);
  bind_function(env, "tsmterm--write-input", fun);

  fun = env->make_function(env, 3, 3, Ftsmterm_set_size,
                           "Sets the size of the terminal.", NULL);
  bind_function(env, "tsmterm--set-size", fun);

  provide(env, "tsmterm-module");
  return 0;
}
