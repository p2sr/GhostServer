.PHONY: all clean

CXX=g++
SDIR=GhostServer
ODIR=obj

SRCS=$(SDIR)/main.cpp $(SDIR)/mainwindow.cpp $(SDIR)/networkmanager.cpp $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp
OBJS=$(patsubst $(SDIR)/%.cpp, $(ODIR)/%.o, $(SRCS))

ODIR_CLI=obj_cli
SRCS_CLI=$(SDIR)/main_cli.cpp $(SDIR)/networkmanager.cpp
OBJS_CLI=$(patsubst $(SDIR)/%.cpp, $(ODIR_CLI)/%.o, $(SRCS_CLI))

DEPS=$(OBJS:%.o=%.d)

CXXFLAGS=-std=c++17 -fPIC -DGHOST_GUI
CXXFLAGS_CLI=-std=c++17 -fPIC
LDFLAGS=-lpthread
LDFLAGS_CLI=-lpthread

include config.mk

all: ghost_server ghost_server_cli
clean:
	rm -rf $(ODIR) $(ODIR_CLI) ghost_server ghost_server_cli
	rm -rf $(SDIR)/ui_mainwindow.h $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp

-include $(DEPS)

ghost_server: $(SDIR)/ui_mainwindow.h $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

ghost_server_cli: $(OBJS_CLI)
	$(CXX) $(OBJS_CLI) $(LDFLAGS_CLI) -o $@

$(ODIR)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

$(ODIR_CLI)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_CLI) -MMD -c $< -o $@

$(SDIR)/ui_mainwindow.h: $(SDIR)/mainwindow.ui
	uic $< >$@

$(SDIR)/%_qt.cpp: $(SDIR)/%.h
	cd $(SDIR); moc $(notdir $<) -o $(notdir $@) -DGHOST_GUI
