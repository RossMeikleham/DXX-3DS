all: physfs d1 d2

physfs:
	@$(MAKE) -C physfs

d1:
	@$(MAKE) -C d1

d2:
	@$(MAKE) -C d1

clean:
	@$(MAKE) -C physfs clean
	@$(MAKE) -C d1 clean

.PHONY: physfs d1 d2
