#ifndef ELISP_H
#define ELISP_H

#include "emacs-module.h"
#include "libtsm.h"

// Emacs symbols
emacs_value Qt;
emacs_value Qnil;
emacs_value Qnormal;
emacs_value Qbold;
emacs_value Qitalic;
emacs_value Qforeground;
emacs_value Qbackground;
emacs_value Qweight;
emacs_value Qunderline;
emacs_value Qslant;
emacs_value Qreverse;
emacs_value Qstrike;
emacs_value Qface;
emacs_value Qcursor_type;

// Emacs functions
emacs_value Flength;
emacs_value Flist;
emacs_value Ferase_buffer;
emacs_value Finsert;
emacs_value Fgoto_char;
emacs_value Fforward_char;
emacs_value Fforward_line;
emacs_value Fgoto_line;
emacs_value Fdelete_lines;
emacs_value Fbuffer_line_number;
emacs_value Frecenter;
emacs_value Fput_text_property;
emacs_value Fset;
emacs_value Ftsmterm_face_color_hex;
emacs_value Ftsmterm_flush_output;
emacs_value Fblink_cursor_mode;

// Utils
void bind_function(emacs_env *env, const char *name, emacs_value Sfun);
void provide(emacs_env *env, const char *feature);
int string_bytes(emacs_env *env, emacs_value string);
emacs_value string_length(emacs_env *env, emacs_value string);
emacs_value list(emacs_env *env, emacs_value *elements, ptrdiff_t len);
void put_text_property(emacs_env *env, emacs_value string, emacs_value property,
                       emacs_value value);
void byte_to_hex(uint8_t byte, char *hex);
/* emacs_value color_to_rgb_string(emacs_env *env, VTermColor color); */
uint8_t hex_to_byte(char *hex);
/* VTermColor rgb_string_to_color(emacs_env *env, emacs_value string); */
void erase_buffer(emacs_env *env);
void insert(emacs_env *env, emacs_value string);
void goto_char(emacs_env *env, int pos);
void forward_line(emacs_env *env, int n);
void goto_line(emacs_env *env, int n);
void toggle_cursor(emacs_env *env, bool visible);
void toggle_cursor_blinking(emacs_env *env, bool visible);
void delete_lines(emacs_env *env, int linenum, int count, bool del_whole_line);
emacs_value get_hex_color_fg(emacs_env *env, emacs_value face);
emacs_value get_hex_color_bg(emacs_env *env, emacs_value face);
emacs_value buffer_line_number(emacs_env *env);
void recenter(emacs_env *env, emacs_value pos);
void forward_char(emacs_env *env, emacs_value n);
#endif /* ELISP_H */
