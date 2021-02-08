.PHONY: all clean

CXX=clang++
SDIR=GhostServer
ODIR=obj

SRCS=$(SDIR)/main.cpp $(SDIR)/mainwindow.cpp $(SDIR)/networkmanager.cpp $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp

OBJS=$(patsubst $(SDIR)/%.cpp, $(ODIR)/%.o, $(SRCS))

DEPS=$(OBJS:%.o=%.d)

CXXFLAGS=-std=c++17 -fPIC
LDFLAGS=-lpthread

include config.mk

all: ghost_server
clean:
	rm -rf $(ODIR) ghost_server
	rm -rf $(SDIR)/ui_mainwindow.h $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp

-include $(DEPS)

ghost_server: $(SDIR)/ui_mainwindow.h $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

$(ODIR)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

$(SDIR)/ui_mainwindow.h: $(SDIR)/mainwindow.ui
	uic $< >$@

$(SDIR)/%_qt.cpp: $(SDIR)/%.h
	cd $(SDIR); moc $(notdir $<) -o $(notdir $@)
