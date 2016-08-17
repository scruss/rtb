
SRC	=	rtb.tex				\
		preface.tex			\
		ch-intro.tex			\
		ch-rtb-environment.tex		\
		ch-command-line.tex		\
		ch-im-commands.tex		\
		ch-comments.tex			\
		ch-flow-control.tex		\
		ch-variables.tex		\
		ch-inputOutput.tex		\
		ch-data.tex			\
		ch-builtin-variables.tex	\
		ch-conditionals.tex		\
		ch-loops.tex			\
		ch-procFns.tex			\
		ch-numericFns.tex		\
		ch-stringFns.tex		\
		ch-graphicProcFns.tex		\
		ch-userProcFn.tex		\
		ap-colours.tex			\
		ap-numbers.tex			\
		ap-fileFns.tex			\
		ap-serial.tex			\
		ap-drc.tex			\


all:		${SRC} rtb.toc index.tex
	@echo	Generating DVI
	@latex  rtb.tex

rtb.toc:	${SRC}
	@echo Regenerating TOC
	@latex  rtb.tex


rtb.dvi:	${SRC}
	@echo	Generating DVI
	@latex  rtb.tex

index.tex:	rtb.idx
	@rm -f index.tex
	@makeindex < rtb.idx > index.tex

pdf:	rtb.dvi
	@dvipdf rtb.dvi


.PHONEY:	clean
clean:
	@rm -f *.dvi *.aux *.log *.ps *.pdf *.toc *.bak *~
	@rm -f index.ilg index.ind rtb.idx
