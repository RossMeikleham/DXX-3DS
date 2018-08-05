all: physfs d1 d2

physfs:
	@$(MAKE) -C physfs

d1: physfs
	@$(MAKE) -C d1

d2: physfs
	@$(MAKE) -C d2

dist-bin: d1 d2
	@$(MAKE) -C d1 dist-bin
	@$(MAKE) -C d2 dist-bin

clean:
	@$(MAKE) -C physfs clean
	@$(MAKE) -C d1 clean
	@$(MAKE) -C d2 clean

.PHONY: physfs d1 d2
