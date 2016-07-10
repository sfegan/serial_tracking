SMALLFIGS = go01.fig go02.fig go03.fig go04.fig go05.fig go06.fig \
            go07.fig go08.fig go09.fig go10.fig go11.fig go12.fig \
            go13.fig go14.fig go15.fig go16.fig go17.fig go18.fig \
            go19.fig go20.fig go21.fig go22.fig go23.fig go24.fig \
            stop.fig

# button1.fig
# button2.fig
# button3.fig

#BIGFIGS = egg01.fig egg02.fig egg03.fig egg04.fig egg05.fig egg06.fig
#          egg07.fig egg08.fig egg09.fig egg10.fig egg11.fig egg12.fig
#          egg13.fig egg14.fig egg15.fig egg16.fig egg17.fig egg18.fig
#          egg19.fig egg20.fig egg21.fig egg22.fig egg23.fig egg24.fig
#          egg25.fig egg26.fig egg27.fig egg28.fig egg29.fig egg30.fig
#          egg31.fig egg32.fig egg33.fig egg34.fig egg35.fig egg36.fig

BIGFIGS = ind_cf01.fig ind_cf02.fig ind_cf03.fig ind_cf04.fig \
          ind_cf05.fig ind_cf06.fig ind_cf07.fig ind_cf08.fig \
          ind_cf09.fig ind_cf10.fig ind_cf11.fig ind_cf12.fig \
          ind_cf13.fig ind_cf14.fig ind_cf15.fig ind_cf16.fig \
          ind_go01.fig ind_go02.fig ind_go03.fig ind_go04.fig \
          ind_go05.fig ind_go06.fig ind_go07.fig ind_go08.fig \
          ind_go09.fig ind_go10.fig ind_go11.fig ind_go12.fig \
          ind_go13.fig ind_go14.fig ind_go15.fig ind_go16.fig \
          ind_go17.fig ind_go18.fig ind_go19.fig ind_go20.fig \
          ind_go21.fig ind_go22.fig ind_go23.fig ind_go24.fig \
          ind_go25.fig ind_go26.fig ind_go27.fig ind_go28.fig \
          ind_go29.fig ind_go30.fig ind_go31.fig ind_go32.fig \
          ind_go33.fig ind_go34.fig ind_go35.fig ind_go36.fig \
          ind_stop.fig padlock.fig

SWFIGS = sw01.fig sw02.fig sw03.fig sw04.fig sw05.fig sw06.fig \
         sw07.fig sw08.fig sw09.fig sw10.fig sw11.fig sw12.fig \
         sw13.fig sw14.fig sw15.fig sw16.fig sw17.fig sw18.fig \
         sw19.fig sw20.fig sw21.fig sw22.fig sw23.fig sw24.fig \
         sw25.fig sw26.fig sw27.fig sw28.fig sw29.fig sw30.fig \
         sw31.fig sw32.fig sw33.fig sw34.fig sw35.fig sw36.fig \
         sw37.fig sw38.fig sw39.fig sw40.fig sw41.fig sw42.fig \
         sw43.fig sw44.fig sw45.fig sw46.fig sw47.fig sw48.fig \
         sw49.fig sw50.fig sw51.fig sw52.fig sw53.fig sw54.fig \
         sw55.fig sw56.fig sw57.fig sw58.fig sw59.fig sw60.fig \
         sw61.fig sw62.fig sw63.fig sw64.fig sw65.fig sw66.fig \
         sw67.fig sw68.fig sw69.fig sw70.fig sw71.fig sw72.fig

all: $(BIGFIGS:.fig=_pix_data.xpm) $(SMALLFIGS:.fig=_pix_data.xpm) \
     $(SWFIGS:.fig=_pix_data.xpm) \
     button1l.xpm button2l.xpm button3l.xpm \
     button1r.xpm button2r.xpm button3r.xpm veritas.xpm

clean:
	$(RM) $(BIGFIGS:.fig=.png) $(SMALLFIGS:.fig=.png) \
             $(SWFIGS:.fig=.png) $(BIGFIGS:.fig=_pix_data.xpm) \
             $(SMALLFIGS:.fig=_pix_data.xpm) $(SWFIGS:.fig=_pix_data.xpm) \
	     button1l.xpm button2l.xpm button3l.xpm \
	     button1r.xpm button2r.xpm button3r.xpm \
             button1.png button2.png button3.png veritas.xpm *~

$(BIGFIGS:.fig=.png): %.png: %.fig
	fig2dev -Lpng $< $@

$(BIGFIGS:.fig=_pix_data.xpm): %_pix_data.xpm: %.png
	convert -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d{2})/;$$temp=$$1; s/__/$$temp/' $@

$(SMALLFIGS:.fig=.png): %.png: %.fig
	fig2dev -Lpng -m2.0 $< $@

$(SMALLFIGS:.fig=_pix_data.xpm): %_pix_data.xpm: %.png
	convert -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d{2})/;$$temp=$$1; s/__/$$temp/' $@

$(SWFIGS:.fig=.png): %.png: %.fig
	fig2dev -Lpng -g \#f0f0f0 $< $@

$(SWFIGS:.fig=_pix_data.xpm): %_pix_data.xpm: %.png
	convert -transparent \#f0f0f0 $< $@
	perl -i -pe '"$@" =~ m/(\d{2})/;$$temp=$$1; s/__/$$temp/' $@

button1r.xpm: button1.png
	convert -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d)/;$$temp=$$1; s/_/$$temp/' $@

button2r.xpm: button2.png
	convert -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d)/;$$temp=$$1; s/_/$$temp/' $@

button3r.xpm: button3.png
	convert -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d)/;$$temp=$$1; s/_/$$temp/' $@

button1l.xpm: button1.png
	convert -flop -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d)/;$$temp=$$1; s/_/$$temp/' $@

button2l.xpm: button2.png
	convert -flop -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d)/;$$temp=$$1; s/_/$$temp/' $@

button3l.xpm: button3.png
	convert -flop -transparent \#ffffff $< $@
	perl -i -pe '"$@" =~ m/(\d)/;$$temp=$$1; s/_/$$temp/' $@

button1.png: button1.fig
	fig2dev -Lpng $< $@

button2.png: button2.fig
	fig2dev -Lpng $< $@

button3.png: button3.fig
	fig2dev -Lpng $< $@

veritas.xpm: logo-Crab.png
	convert $< $@
