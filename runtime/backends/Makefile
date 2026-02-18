SUBDIR := cpu verilog

all:  # set default target as 'all'

%:
	@for dir in $(SUBDIR); do \
		$(MAKE) -C $$dir $@; \
	done
