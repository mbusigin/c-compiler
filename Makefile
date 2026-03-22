# Makefile for /Users/mbusigin/c-compiler - delegates to project Makefile
.PHONY: test
test:
	$(MAKE) -C project test
