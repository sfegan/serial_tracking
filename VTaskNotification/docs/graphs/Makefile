
DOTS = Task.dot Notification.dot QtAsyncAlerter.dot TaskNotification.dot
PS   = $(DOTS:.dot=.ps)
PNG  = $(DOTS:.dot=.png)

all: $(PNG) $(PS)

clean:
	$(RM) -f $(PS) $(PNG) *~

%.ps: %.dot
	dot -Tps -o $@ $<

%.png: %.dot
	dot -Tpng $< | convert -geometry 60x60% - $@
#	dot -Tpng -o $@ $<
