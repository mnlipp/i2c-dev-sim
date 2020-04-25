TOPTARGETS := all clean

SUBDIRS := i2c-virt-bus-test

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@/Debug $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
