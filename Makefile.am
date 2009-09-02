# require the minimal version of automake 1.7
AUTOMAKE_OPTIONS = foreign 1.7

SUBDIRS = src

EXTRA_DIST = apvlv.1 apvlvrc.example Startup.pdf icons/*

pdf_DATA = Startup.pdf
doc_DATA = apvlvrc.example icons/reg.png icons/dir.png icons/pdf.png
man_MANS = apvlv.1

install-exec-local:
	mkdir -p $(sysconfdir)
	$(INSTALL) -m 0644 apvlvrc.example $(sysconfdir)/apvlvrc
