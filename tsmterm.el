;;; tsmterm.el --- This package implements a terminal via libtsm

;;; Commentary:
;;
;; This Emacs module implements a bridge to libtsmterm to display a terminal in a
;; Emacs buffer.

;;; Code:

(require 'tsmterm-module)
(require 'subr-x)
(require 'cl-lib)
(require 'color)

(defcustom tsmterm-shell (getenv "SHELL")
  "The shell that gets run in the tsmterm."
  :type 'string
  :group 'tsmterm)

(defcustom tsmterm-max-scrollback 1000
  "Maximum 'scrollback' value."
  :type 'number
  :group 'tsmterm)

(defcustom tsmterm-keymap-exceptions '("C-x" "C-u" "C-g" "C-h" "M-x" "M-o" "C-v" "M-v")
  "Exceptions for tsmterm-keymap.

If you use a keybinding with a prefix-key that prefix-key cannot
be send to the terminal."
  :type '(repeat string)
  :group 'tsmterm)

(defface tsmterm
  '((t :inherit default))
  "Default face to use in Term mode."
  :group 'tsmterm)

(defface tsmterm-color-black
  '((t :foreground "black" :background "black"))
  "Face used to render black color code."
  :group 'tsmterm)

(defface tsmterm-color-red
  '((t :foreground "red3" :background "red3"))
  "Face used to render red color code."
  :group 'tsmterm)

(defface tsmterm-color-green
  '((t :foreground "green3" :background "green3"))
  "Face used to render green color code."
  :group 'tsmterm)

(defface tsmterm-color-yellow
  '((t :foreground "yellow3" :background "yellow3"))
  "Face used to render yellow color code."
  :group 'tsmterm)

(defface tsmterm-color-blue
  '((t :foreground "blue2" :background "blue2"))
  "Face used to render blue color code."
  :group 'tsmterm)

(defface tsmterm-color-magenta
  '((t :foreground "magenta3" :background "magenta3"))
  "Face used to render magenta color code."
  :group 'tsmterm)

(defface tsmterm-color-cyan
  '((t :foreground "cyan3" :background "cyan3"))
  "Face used to render cyan color code."
  :group 'tsmterm)

(defface tsmterm-color-white
  '((t :foreground "white" :background "white"))
  "Face used to render white color code."
  :group 'tsmterm)

(defvar tsmterm--term nil
  "Pointer to Term.")
(make-variable-buffer-local 'tsmterm--term)

(defvar tsmterm--process nil
  "Shell process of current term.")
(make-variable-buffer-local 'tsmterm--process)

(defvar tsmterm--timer nil
  "Timer to update the term.")
(make-variable-buffer-local 'tsmterm--timer)

(defvar tsmterm--timer-interval 0.05
  "")

(define-derived-mode tsmterm-mode fundamental-mode "Tsm"
  "Mayor mode for tsmterm buffer."
  (buffer-disable-undo)
  (setq tsmterm--term (tsmterm--new (window-body-height)
                                    (window-body-width)
                                    tsmterm-max-scrollback))
  ;; (cl-loop repeat (1- (window-body-height)) do
  ;;          (insert "\n"))

  (setq buffer-read-only t)
  (setq-local scroll-conservatively 101)
  (setq-local scroll-margin 0)

  (add-hook 'window-size-change-functions #'tsmterm--window-size-change t t)
  (let ((process-environment (append '("TERM=xterm") process-environment)))
    (setq tsmterm--process (make-process
                            :name "tsmterm"
                            :buffer (current-buffer)
                            :command `("/bin/sh" "-c" ,(format "stty -nl sane iutf8 rows %d columns %d >/dev/null && exec %s" (window-body-height) (window-body-width) tsmterm-shell))
                            :coding 'no-conversion
                            :connection-type 'pty
                            :filter #'tsmterm--filter
                            :sentinel #'ignore))
    ;; (setq tsmterm--timer (run-with-timer 0 tsmterm--timer-interval #'tsmterm--run-timer buffer))
    (add-hook 'kill-buffer-hook #'tsmterm--kill-buffer-hook t t)))


;; Keybindings
(define-key tsmterm-mode-map [t] #'tsmterm--self-insert)
(define-key tsmterm-mode-map [mouse-1] nil)
(define-key tsmterm-mode-map [mouse-2] nil)
(define-key tsmterm-mode-map [mouse-3] nil)
(define-key tsmterm-mode-map [mouse-4] #'ignore)
(define-key tsmterm-mode-map [mouse-5] #'ignore)
(dolist (prefix '("M-" "C-"))
  (dolist (char (cl-loop for char from ?a to ?z
                         collect char))
    (let ((key (concat prefix (char-to-string char))))
      (unless (cl-member key tsmterm-keymap-exceptions)
        (define-key tsmterm-mode-map (kbd key) #'tsmterm--self-insert)))))
(dolist (exception tsmterm-keymap-exceptions)
  (define-key tsmterm-mode-map (kbd exception) nil))

(defun tsmterm--self-insert ()
  "Sends invoking key to libtsm."
  (interactive)
  (when tsmterm--term
    (let* ((modifiers (event-modifiers last-input-event))
           (shift (memq 'shift modifiers))
           (meta (memq 'meta modifiers))
           (ctrl (memq 'control modifiers)))
      (when-let ((key (key-description (vector (event-basic-type last-input-event))))
                 (inhibit-redisplay t)
                 (inhibit-read-only t))
        (when (equal modifiers '(shift))
          (setq key (upcase key)))
        (tsmterm--update tsmterm--term key shift meta ctrl)))))

(defun tsmterm ()
  "Create a new tsmterm."
  (interactive)
  (let ((buffer (generate-new-buffer "tsmterm")))
    (with-current-buffer buffer
      (tsmterm-mode))
    (switch-to-buffer buffer)))

(defun tsmterm-other-window ()
  "Create a new tsmterm."
  (interactive)
  (let ((buffer (generate-new-buffer "tsmterm")))
    (with-current-buffer buffer
      (tsmterm-mode))

    (pop-to-buffer buffer)))

(defun tsmterm--flush-output (output)
  "Sends the virtual terminal's OUTPUT to the shell."
  (process-send-string tsmterm--process output))

(defun tsmterm--filter (process input)
  "I/O Event. Feeds PROCESS's INPUT to the virtual terminal.

Then triggers a redraw from the module."
  (with-current-buffer (process-buffer process)
    (let ((inhibit-redisplay t)
          (inhibit-read-only t))
      (tsmterm--write-input tsmterm--term input)
      (tsmterm--update tsmterm--term))))


(defun tsmterm--run-timer (buffer)
  (interactive)
  (let ((inhibit-redisplay t)
        (inhibit-read-only t))
    (when (buffer-live-p buffer)
      (with-current-buffer buffer
        (tsmterm--update tsmterm--term)))))


(defun tsmterm--kill-buffer-hook ()
  (when (and (eq major-mode 'tsmterm-mode)
         tsmterm--timer)
    (cancel-timer tsmterm--timer)))

(defun tsmterm--window-size-change (frame)
  "Callback triggered by a size change of the FRAME.

Feeds the size change to the virtual terminal."
  (dolist (window (window-list frame))
    (with-current-buffer (window-buffer window)
      (when (and (processp tsmterm--process)
                 (process-live-p tsmterm--process))
        (let ((height (window-body-height window))
              (width (window-body-width window)))
          (set-process-window-size tsmterm--process height width)
          (tsmterm--set-size tsmterm--term height width))))))

(defun tsmterm--face-color-hex (face attr)
  "Return the color of the FACE's ATTR as a hex string."
  (if (< emacs-major-version 26)
      (apply #'color-rgb-to-hex (color-name-to-rgb (face-attribute face attr nil 'default)))
    (apply #'color-rgb-to-hex (append (color-name-to-rgb (face-attribute face attr nil 'default)) '(2)))))


(defun tsmterm--delete-lines (line-num count &optional delete-whole-line)
  "Delete lines from line-num. If option ‘kill-whole-line’ is non-nil,
 then this command kills the whole line including its terminating newline"
  (ignore-errors
    (save-excursion
      (when (tsmterm--goto-line line-num)
        (delete-region (point) (point-at-eol))
        (when delete-whole-line
          (when (looking-at "\n")
            (delete-char 1))
          (when (and (eobp) (looking-back "\n"))
            (delete-char -1)))
        (cl-loop repeat (1- count) do
                 (when (or delete-whole-line (forward-line 1))
                   (delete-region (point) (point-at-eol))
                   (when delete-whole-line
                     (when (looking-at "\n")
                       (delete-char 1))
                     (when (and (eobp) (looking-back "\n"))
                       (delete-char -1)))))))))



(defun tsmterm--recenter(&optional arg)
  (when (get-buffer-window)
    (with-current-buffer (window-buffer)
      (when tsmterm--term
        (recenter arg)))))

(defun tsmterm--goto-line(n)
  "If move succ return t"
  (ignore-errors
    (goto-char (point-min))
    (let ((succ (eq 0 (forward-line (1- n)))))
      succ)))

(defun tsmterm--buffer-line-num()
  (ignore-errors
    (save-excursion
      (goto-char (point-max))
      (line-number-at-pos))))

(defun tsmterm--forward-char(n )
  (ignore-errors
    (forward-char n)))

(provide 'tsmterm)
;;; tsmterm.el ends here
