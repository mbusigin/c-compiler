# Makefile for /Users/mbusigin/c-compiler - delegates to project Makefile
.PHONY: test self self-verify help

test:
	$(MAKE) -C project test

# Self-hosting bootstrap: compile the compiler with itself
self:
	$(MAKE) -C project self

# Verify bootstrap convergence (stage1 == stage2)
self-verify:
	$(MAKE) -C project self-verify

# Help information
help:
	@echo "C Compiler Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make            - Build the compiler (default)"
	@echo "  make test       - Run all tests"
	@echo "  make self       - Self-hosting bootstrap test (compile compiler with itself)"
	@echo "  make self-verify - Verify bootstrap convergence"
	@echo "  make clean      - Remove build artifacts"
	@echo ""
	@echo "Self-hosting status:"
	@echo "  The compiler is working toward self-hosting capability."
	@echo "  Currently missing: preprocessor, struct member access, sizeof, typedef."
