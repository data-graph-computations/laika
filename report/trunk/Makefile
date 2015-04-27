#DOC = $(notdir $(shell pwd))
DOC=progress_report

# This line is intentionally blank.

OTHERS = Makefile

DEPS_DIR = .deps

LATEXMK=latexmk -pdf -dvi- -ps- -recorder -deps \
                  -pdflatex='pdflatex --shell-escape %O %S' \
                  -e 'show_cus_dep();' \
                  # -e 'warn qq(In Makefile, turn off custom dependencies);' \


default: pdf
.PHONY:  pdf default all remake clean test

test:
	echo $(DOC)

$(DEPS_DIR) :
	mkdir $@


%.w18:
	touch $@

###########################################################################
## Make targets for cleaning

clean:
	rm -f $(DOC).pdf $(DOC).aux $(DOC).log $(DOC).out $(DOC).brf $(DOC).code $(DOC).code.out $(DOC).fdb_latexmk $(DOC).fls $(DOC).bbl $(DOC).blg .deps/$(DOC).pdfP *~


###########################################################################
## Make targets for document

pdf : $(DOC).pdf

-include $(DEPS_DIR)/$(DOC).pdfP

%.w18 :
	touch $@

%.pdf : %.tex $(OTHERS)
	if [ ! -e $(DEPS_DIR) ]; then mkdir $(DEPS_DIR); fi
	$(LATEXMK) -deps-out=$(DEPS_DIR)/$@P $<
	if [ -e $@ ]; then touch $@; fi

remake :
	if [ ! -e $(DEPS_DIR) ]; then mkdir $(DEPS_DIR); fi
	$(LATEXMK) -g -deps-out=$(DEPS_DIR)/$(DOC).pdfP $(DOC).tex
	if [ -e $(DOC).pdf ]; then touch $(DOC).pdf; fi
