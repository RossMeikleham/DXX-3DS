all: d1 d2

d1:
	@$(MAKE) -C d1

d2:
	@$(MAKE) -C d2

dist-bin: d1 d2
	@$(MAKE) -C d1 dist-bin
	@$(MAKE) -C d2 dist-bin

clean:
	@$(MAKE) -C d1 clean
	@$(MAKE) -C d2 clean

.PHONY: d1 d2
