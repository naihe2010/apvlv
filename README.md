# apvlv

apvlv is a PDF/EPUB/TXT/FB2/MOBI/CBZ/HTML ... Viewer Under Linux/WIN32 and its behaviour like Vim.

Apvlv is an open source software, was created by Alf and is hosted on [github](https://github.com/naihe2010/apvlv).

Now, it is still growing.

Like Vim, Apvlv makes people reading their PDF/EPUB files just like using Vim.

So, Apvlv bindings lots of Vim command and its behaviour is like Vim. For example, < Ctrl-f > to forward page, < Ctrl-b > to previous page, 'k','j','h','l' to scrolling a page up, down, left or right, and so on.

And, Apvlv can understand that how many times you want to run the command.

The only thing you need to do is typing the number before the command. For example, typing '50' and < Ctrl-f > will go forward 50 pages, typing '30' and < Ctrl-b > will go previous 30 pages.

What's more import is apvlv can support view a directory as content of a pdf/epub document. Pressing 'k' or 'j' to move selected up or down, 'h' or 'l' to collapse or expand a dir, and press 't' will open the selected document in a new tab.

# Dependencies

+ Qt6 ( http://www.qt.org/ )
+ Quazip ( https://github.com/stachenov/quazip )

# Optional Dependencies

+ Poppler-qt6 ( https://www.qt.io/ )
+ MuPDF ( https://www.mupdf.com/ )
+ Teeseract ( https://github.com/tesseract-ocr/tessdoc )

# Download

+ Releases (https://github.com/naihe2010/apvlv/releases)

# Build

1. Using cmake to generate Makefile.
   
   ```
   cmake .
   ```
2. Execute make.
   
   ```
   make
   ```

# Install

+ Make a package and install it.
  
  ```
  make package
  ```
+ Or install it directly.
  
  ```
  sudo make install
  ```

# License

apvlv is licensed under the GNU General Public License (GPL).

# Contact

+ Email: Alf [naihe2010@126.com](mailto:naihe2010@126.com)

# Develop Tools

+ Vim (https://www.vim.org/)

+ Emacs (https://www.gnu.org/software/emacs/)

+ CLion (https://www.jetbrains.com/clion/)
  
  Thanks these great tools.

# Github Actions
![CI Status](https://github.com/naihe2010/apvlv/actions/workflows/cmake-multi-platform.yml/badge.svg)
