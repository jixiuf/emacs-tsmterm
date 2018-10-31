#include "tsmterm-module.h"
#include "elisp.h"
#include "utf8.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#ifdef BUILD_HAVE_XKBCOMMON
#  include <xkbcommon/xkbcommon-keysyms.h>
#else
#  include "xkbcommon-keysyms.h"
#endif

FILE *logfile;


static void err(const char *format, ...) {
  va_list list;

  fprintf(logfile, "ERROR: ");

  va_start(list, format);
  vfprintf(logfile, format, list);
  va_end(list);

  fprintf(logfile, "\n");
  fflush(logfile);
}


static void info(const char *format, ...) {

	va_list list;

	fprintf(logfile, "INFO: ");

	va_start(list, format);
	vfprintf(logfile, format, list);
	va_end(list);

	fprintf(logfile, "\n");
    fflush(logfile);
}

static const char *sev2str_table[] = {
	"FATAL",
	"ALERT",
	"CRITICAL",
	"ERROR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG"
};

static const char *sev2str(unsigned int sev) {
	if (sev > 7)
		return "DEBUG";

	return sev2str_table[sev];
}

static void log_tsm(void *data, const char *file, int line, const char *fn,
		    const char *subs, unsigned int sev, const char *format,
		    va_list args) {
	fprintf(logfile, "%s: %s: ", sev2str(sev), subs);
	vfprintf(logfile, format, args);
	fprintf(logfile, "\n");
    fflush(logfile);
}



static void log_tsm_init(){
  logfile = fopen("/tmp/emacs-tsmterm.log", "w+");
  info("init log");

}

static emacs_value render_text(emacs_env *env, char *buffer, int len,
                               const struct tsm_screen_attr *attr) {
  emacs_value text;
  if (len == 0) {
    text = env->make_string(env, "", 0);
    return text;
  } else {
    text = env->make_string(env, buffer, len);
  }

  emacs_value foreground = color_to_rgb_string(env, attr->fr,attr->fg,attr->fb);
  emacs_value background = color_to_rgb_string(env, attr->br,attr->bg,attr->bb);
  emacs_value bold = attr->bold ? Qbold : Qnormal;
  emacs_value underline = attr->underline ? Qt : Qnil;
  /* emacs_value italic = attr->italic ? Qitalic : Qnormal; */
  emacs_value reverse = attr->inverse ? Qt : Qnil;
  /* emacs_value strike = attr->strike ? Qt : Qnil; */
  emacs_value blink = attr->blink? Qt : Qnil;

  // TODO: Blink, font, dwl, dhl is missing
  emacs_value properties =
    list(env,
         (emacs_value[]){Qforeground, foreground,
                         Qbackground, background,
                         Qweight, bold,
                         Qunderline, underline,
                         /* Qslant, italic, */
                         Qreverse, reverse
                         /* Qstrike, strike */
         },
         10);

  put_text_property(env, text, Qface, properties);

  return text;
}

static int term_draw_cell(struct tsm_screen *screen, uint32_t id,
                          const uint32_t *ch, size_t len,
                          unsigned int cwidth, unsigned int posx,
                          unsigned int posy,
                          const struct tsm_screen_attr *attr,
                          tsm_age_t age, void *data)
{
  Term *term = data;
  info("ce..age=%d,lastage=%d",age,term->age);
  uint8_t fr, fg, fb, br, bg, bb;
  unsigned int x, y;
  int r;
  emacs_env *env=term->env;
  char *ptr=term->outputbuf;
  char *ptr2;
  ptr+=term->outputbuf_len;
  ptr2=ptr;
  int i;
  char buffer[4];
  if(len==0){
    strncpy(ptr," ",1);
    term->outputbuf_len+=1;
    ptr+=1;
    emacs_value text=env->make_string(env, " ", 1);
    /* emacs_value text=render_text(env," ",1,attr); */
    insert(env,text);
  }else{
    for (i = 0; i < len; i++){
      int size =tsm_ucs4_to_utf8(ch[i],buffer);
      if(size){
        strncpy(ptr,buffer,size);
        term->outputbuf_len+=size;
        /* emacs_value text=env->make_string(env, ptr, size); */
        emacs_value text=render_text(env,ptr,size,attr);
        insert(env,text);
        ptr+=size;
      }
    }
  }
  if(posx==term->width-1&&posy!=term->height-1){
    strncpy(ptr,"\n",1);
    term->outputbuf_len+=1;
        emacs_value text=env->make_string(env, "\n", 1);
        insert(env,text);
  }
  term->outputbuf[term->outputbuf_len]='\0';

  /* info("drawcell len=%d,x=%d,y=%d,width=%d height=%d |%s| %d", */
       /* len,posx,posy,term->width,term->height,ptr2,term->outputbuf_len); */
  return 0;
}
static void term_redraw(Term *term, emacs_env *env) {
  struct tsm_screen_attr attr;
  unsigned int w, h;

  term->env=env;
  erase_buffer(env);
  term->outputbuf_len=0;
  info("redraw init term->outputbuf_len=0");
  /* insert(env,env->make_string(env, "hello", 5)); */
  term->age = tsm_screen_draw(term->screen, term_draw_cell,
                            (void*)term);
  term->outputbuf[term->outputbuf_len]='\0';
  info("draw  %s %d age=%d",term->outputbuf,term->outputbuf_len,term->age);
  /* w = tsm_screen_get_width(term->screen); */
  /* h = tsm_screen_get_height(term->screen); */
  /* tsm_vte_get_def_attr(term->vte, &attr); */
  /* info("term_redraw age=%d",age); */

  /* insert(env,env->make_string(env, term->outputbuf, term->outputbuf_len)); */
  term->outputbuf_len=0;
  /* goto_line(env,1); */
}

static emacs_value Ftsmterm_write_input(emacs_env *env, ptrdiff_t nargs,
                                        emacs_value args[], void *data) {
  /* write the output of shell to terminal ,
     so that the terminal can render and display it .
  */
  Term *term = env->get_user_ptr(env, args[0]);
  ptrdiff_t len = string_bytes(env, args[1]);
  char bytes[len];

  env->copy_string_contents(env, args[1], bytes, &len);
  tsm_vte_input(term->vte, bytes, len);

  return env->make_integer(env, 0);
}

static bool is_key(unsigned char *key, size_t len, char *key_description) {
  return (len == strlen(key_description) &&
          memcmp(key, key_description, len) == 0);
}




static void term_process_key(Term *term, unsigned char *key, size_t len,
                             unsigned int  modifier) {
  uint32_t keysym=0;
  if (is_key(key, len, "<return>")) {
    keysym= XKB_KEY_Return;
  } else if (is_key(key, len, "<tab>")) {
    keysym= XKB_KEY_Tab;
  } else if (is_key(key, len, "<backspace>")) {
    keysym= XKB_KEY_BackSpace;
  } else if (is_key(key, len, "<escape>")) {
    keysym= XKB_KEY_Escape;
  } else if (is_key(key, len, "<up>")) {
    keysym= XKB_KEY_Up;
  } else if (is_key(key, len, "<down>")) {
    keysym= XKB_KEY_Down;
  } else if (is_key(key, len, "<left>")) {
    keysym= XKB_KEY_Left;
  } else if (is_key(key, len, "<right>")) {
    keysym= XKB_KEY_Right;
  } else if (is_key(key, len, "<insert>")) {
    keysym= XKB_KEY_Insert;
  } else if (is_key(key, len, "<delete>")) {
    keysym= XKB_KEY_Delete;
  } else if (is_key(key, len, "<home>")) {
    keysym= XKB_KEY_Home;
  } else if (is_key(key, len, "<end>")) {
    keysym= XKB_KEY_End;
  } else if (is_key(key, len, "<prior>")) {
    keysym= XKB_KEY_Prior;
  } else if (is_key(key, len, "<f1>")) {
    keysym= XKB_KEY_F1;
  } else if (is_key(key, len, "<f2>")) {
    keysym= XKB_KEY_F2;
  } else if (is_key(key, len, "<f3>")) {
    keysym= XKB_KEY_F3;
  } else if (is_key(key, len, "<f4>")) {
    keysym= XKB_KEY_F4;
  } else if (is_key(key, len, "<f5>")) {
    keysym= XKB_KEY_F5;
  } else if (is_key(key, len, "<f6>")) {
    keysym= XKB_KEY_F6;
  } else if (is_key(key, len, "<f7>")) {
    keysym= XKB_KEY_F7;
  } else if (is_key(key, len, "<f8>")) {
    keysym= XKB_KEY_F8;
  } else if (is_key(key, len, "<f9>")) {
    keysym= XKB_KEY_F9;
  } else if (is_key(key, len, "<f10>")) {
    keysym= XKB_KEY_F10;
  } else if (is_key(key, len, "<f11>")) {
    keysym= XKB_KEY_F11;
  } else if (is_key(key, len, "<f12>")) {
    keysym= XKB_KEY_F12;
  } else if (is_key(key, len, "SPC")) {
    keysym= XKB_KEY_KP_Space;
  }
  uint32_t codepoint=0;
  if (len <= 4) {
    if (!utf8_to_codepoint(key, len, &codepoint)) {
      codepoint = TSM_VTE_INVALID;
    }
  }
  if (tsm_vte_handle_keyboard(term->vte, keysym, 0, modifier, codepoint)) {
    tsm_screen_sb_reset(term->screen);
    return ;
  }

}


static void term_finalize(void *object) {
  Term *term = (Term *)object;
  tsm_vte_unref(term->vte);
  tsm_screen_unref(term->screen);
  free(term);
}
static void term_flush_output(Term *term, emacs_env *env) {
  /* flush what user input to shell */
  if (term->outputbuf_len>0) {
    info("term_flush_output len=%d,data=%s",term->outputbuf_len,term->outputbuf);
    emacs_value output = env->make_string(env, term->outputbuf, term->outputbuf_len);
    env->funcall(env, Ftsmterm_flush_output, 1, (emacs_value[]){output});
    term->outputbuf_len=0;
  }
}

static void term_write_cb(struct tsm_vte *vte, const char *u8, size_t len,
                          void *data) {
  /* handle user input  */
  Term *term = data;
  int r;

  if (!term->initialized){
    return;
  }
  if(len>0){
    char *ptr=term->outputbuf;
    ptr+=term->outputbuf_len;
    strcpy(ptr, u8);
    term->outputbuf_len+=len;
    ptr+=len;
    *ptr='\0';
    info("term_write_cb data2=%s",term->outputbuf);

  }
  /* todo:make sure  term->outputbuf is big enough*/

}
 static uint8_t color_palette[COLOR_NUM][3] = {
 	[COLOR_BLACK]         = { 0x00, 0x00, 0x00 },
 	[COLOR_RED]           = { 0xab, 0x46, 0x42 },
 	[COLOR_GREEN]         = { 0xa1, 0xb5, 0x6c },
 	[COLOR_YELLOW]        = { 0xf7, 0xca, 0x88 },
 	[COLOR_BLUE]          = { 0x7c, 0xaf, 0xc2 },
 	[COLOR_MAGENTA]       = { 0xba, 0x8b, 0xaf },
 	[COLOR_CYAN]          = { 0x86, 0xc1, 0xb9 },
 	[COLOR_LIGHT_GREY]    = { 0xaa, 0xaa, 0xaa },
 	[COLOR_DARK_GREY]     = { 0x55, 0x55, 0x55 },
 	[COLOR_LIGHT_RED]     = { 0xab, 0x46, 0x42 },
 	[COLOR_LIGHT_GREEN]   = { 0xa1, 0xb5, 0x6c },
	[COLOR_LIGHT_YELLOW]  = { 0xf7, 0xca, 0x88 },
 	[COLOR_LIGHT_BLUE]    = { 0x7c, 0xaf, 0xc2 },
 	[COLOR_LIGHT_MAGENTA] = { 0xba, 0x8b, 0xaf },
 	[COLOR_LIGHT_CYAN]    = { 0x86, 0xc1, 0xb9 },
 	[COLOR_WHITE]         = { 0xff, 0xff, 0xff },

 	[COLOR_FOREGROUND]    = { 0x18, 0x18, 0x18 },
 	[COLOR_BACKGROUND]    = { 0xd8, 0xd8, 0xd8 },
 };

static void term_set_palette_colors(Term *term, int colorId, uint8_t r,uint8_t g ,uint8_t b) {
  color_palette[colorId][0]=r;
  color_palette[colorId][1]=g;
  color_palette[colorId][2]=b;
  /* libtsm doesnot support now */
}
static void term_setup_colors(Term *term, emacs_env *env) {
  struct tsm_screen_attr attr;

  rgb_string_to_color(env, get_hex_color_fg(env, Qterm),&attr.fr,&attr.fg,&attr.fb);
  rgb_string_to_color(env, get_hex_color_bg(env, Qterm),&attr.br,&attr.bg,&attr.bb);
  tsm_screen_set_def_attr(term->screen, &attr);
  term_set_palette_colors(term, COLOR_FOREGROUND, attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_BACKGROUND, attr.br,attr.bg,attr.bb);

  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_black),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_BLACK, attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_LIGHT_GREY, attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_DARK_GREY, attr.fr,attr.fg,attr.fb);


  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_red),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_RED,attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_LIGHT_RED,attr.fr,attr.fg,attr.fb);


  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_green),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_GREEN,attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_LIGHT_GREEN,attr.fr,attr.fg,attr.fb);

  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_yellow),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_YELLOW,attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_LIGHT_YELLOW,attr.fr,attr.fg,attr.fb);

  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_blue),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_BLUE,attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_LIGHT_BLUE,attr.fr,attr.fg,attr.fb);

  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_magenta),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_MAGENTA,attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_LIGHT_MAGENTA,attr.fr,attr.fg,attr.fb);

  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_cyan),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_CYAN,attr.fr,attr.fg,attr.fb);
  term_set_palette_colors(term, COLOR_LIGHT_CYAN,attr.fr,attr.fg,attr.fb);

  rgb_string_to_color(env, get_hex_color_fg(env, Qterm_color_white),&attr.fr,&attr.fg,&attr.fb);
  term_set_palette_colors(term, COLOR_WHITE,attr.fr,attr.fg,attr.fb);
  tsm_vte_set_custom_palette(term->vte,color_palette);
  tsm_vte_set_palette(term->vte,"custom");
}

static emacs_value Ftsmterm_new(emacs_env *env, ptrdiff_t nargs,
                                emacs_value args[], void *data) {
  Term *term = malloc(sizeof(Term));
  int rows = env->extract_integer(env, args[0]);
  int cols = env->extract_integer(env, args[1]);
  int sb_size = env->extract_integer(env, args[2]);

  int r;
  r = tsm_screen_new(&term->screen, log_tsm, term);
  if (r < 0)
    goto err_free;

  tsm_screen_resize(term->screen,cols,rows);
  tsm_screen_set_max_sb(term->screen, sb_size > 0 ? sb_size : 0);

  r = tsm_vte_new(&term->vte, term->screen, term_write_cb, term,
                  log_tsm, term);
  if (r < 0)
    goto err_screen;


  term->adjust_size = 1;
  term->initialized=1;
  term->outputbuf_len=0;
  term->width=cols;
  term->height=rows;
  term_setup_colors(term,env);
  return env->make_user_ptr(env, term_finalize, term);

 err_vte:
  tsm_vte_unref(term->vte);
 err_screen:
  tsm_screen_unref(term->screen);
 /* err_font: */
 /*  wlt_font_unref(term->font); */
 err_free:
  free(term);

  return env->make_user_ptr(env, NULL, NULL);
}

static emacs_value Ftsmterm_update(emacs_env *env, ptrdiff_t nargs,
                                   emacs_value args[], void *data) {
  Term *term = env->get_user_ptr(env, args[0]);
  // Process keys
  if (nargs > 1) {
    ptrdiff_t len = string_bytes(env, args[1]);
    unsigned char key[len];
    env->copy_string_contents(env, args[1], (char *)key, &len);
    uint32_t modifier =0;
    if (env->is_not_nil(env, args[2]))
      modifier = modifier | TSM_SHIFT_MASK;
    if (env->is_not_nil(env, args[3]))
      modifier = modifier | TSM_ALT_MASK;
    if (env->is_not_nil(env, args[4]))
      modifier = modifier | TSM_CONTROL_MASK;

    // Ignore the final zero byte
    term_process_key(term, key, len - 1, modifier);
  }

  // Flush output
  term_flush_output(term, env);

  term_redraw(term, env);

  return env->make_integer(env, 0);
}


static emacs_value Ftsmterm_set_size(emacs_env *env, ptrdiff_t nargs,
                                   emacs_value args[], void *data) {
  Term *term = env->get_user_ptr(env, args[0]);
  int rows = env->extract_integer(env, args[1]);
  int cols = env->extract_integer(env, args[2]);
  term->width=cols;
  term->height=rows;
  tsm_screen_resize(term->screen,cols,rows);


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
  log_tsm_init();
  return 0;
}
