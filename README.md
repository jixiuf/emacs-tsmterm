# Introduction

This emacs module implements a bridge to libtsm to display a terminal in a
emacs buffer.

## Warning

This is not finished, so it will crash your emacs. If it does, please
report a bug!

# Installation

```
git clone https://github.com/akermu/emacs-tsmterm.git
```

```
mkdir -p build
cd build
cmake ..
make
```

And add this to your `init.el`:

```
(add-to-list 'load-path "path/to/emacs-tsmterm")
(require 'tsmterm)
```

If you want to have the module compiled, wrap the call to `require` as follows:

```
(add-to-list 'load-path "path/to/emacs-tsmterm")
(require 'tsmterm)

```

# Debugging and testing

If you have successfully build the module, you can test the module by executing
the following command in the `build` directory:

```
make run
```

# Usage

## `tsmterm`

Open a terminal in the current window.

## `tsmterm-other-window`

Open a terminal in another window.

# Customization

## `tsmterm-shell`

Shell to run in a new tsmterm. Defaults to `$SHELL`.

## `tsmterm-keymap-exceptions`

List of keys, which should be processed by emacs and not by the terminal.

## Colors

Set the `:foreground` and `:background` attributes of the following faces to a
color you like:

- tsmterm
- tsmterm-color-black
- tsmterm-color-red
- tsmterm-color-green
- tsmterm-color-yellow
- tsmterm-color-blue
- tsmterm-color-magenta
- tsmterm-color-cyan
- tsmterm-color-white
