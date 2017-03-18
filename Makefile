# makefile for building game client
# see INSTALL for installation instructions

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= none

# Utilities.
MKDIR= mkdir

# == END OF USER SETTINGS. NO NEED TO CHANGE ANYTHING BELOW THIS LINE =========

# Convenience platforms targets.
PLATS= linux macosx generic

all:	$(PLAT)

$(PLATS) clean test:
	cd lua-5.1.4 && $(MAKE) $@
	cd client && $(MAKE) $@

none:
	@echo "Please do"
	@echo "   make PLATFORM"
	@echo "where PLATFORM is one of these:"
	@echo "   $(PLATS)"
	@echo "See INSTALL for complete instructions."

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all $(PLATS) clean test none

# (end of Makefile)
