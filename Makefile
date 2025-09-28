.PHONY: all clean

CXX=g++
SDIR=GhostServer

ODIR_GUI=obj_gui
SRCS_GUI=$(SDIR)/main.cpp $(SDIR)/mainwindow.cpp $(SDIR)/networkmanager.cpp $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp
OBJS_GUI=$(patsubst $(SDIR)/%.cpp, $(ODIR_GUI)/%.o, $(SRCS_GUI))

ODIR_CLI=obj_cli
SRCS_CLI=$(SDIR)/main_cli.cpp $(SDIR)/networkmanager.cpp
OBJS_CLI=$(patsubst $(SDIR)/%.cpp, $(ODIR_CLI)/%.o, $(SRCS_CLI))

DEPS=$(OBJS_GUI:%.o=%.d)

CXXFLAGS=-std=c++17 -fPIC -Ilib/SFML/include -DSFML_STATIC
CXXFLAGS_GUI=-DGHOST_GUI
CXXFLAGS_CLI=

LDFLAGS=-lpthread -Llib/SFML/lib/linux -lsfml-network -lsfml-system
LDFLAGS_GUI=
LDFLAGS_CLI=

CXXFLAGS_GUI += $(shell pkg-config --cflags Qt5Widgets)
LDFLAGS_GUI += $(shell pkg-config --libs Qt5Widgets)

-include config.mk

all: ghost_server ghost_server_cli
clean:
	rm -rf $(ODIR_GUI) $(ODIR_CLI) ghost_server ghost_server_cli
	rm -rf $(SDIR)/ui_mainwindow.h $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp

-include $(DEPS)

ghost_server: $(SDIR)/ui_mainwindow.h $(OBJS_GUI)
	$(CXX) $(OBJS_GUI) $(LDFLAGS) $(LDFLAGS_GUI) -o $@

ghost_server_cli: $(OBJS_CLI)
	$(CXX) $(OBJS_CLI) $(LDFLAGS) $(LDFLAGS_CLI) -o $@

$(ODIR_GUI)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_GUI) -MMD -c $< -o $@

$(ODIR_CLI)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_CLI) -MMD -c $< -o $@

$(SDIR)/ui_mainwindow.h: $(SDIR)/mainwindow.ui
	uic $< >$@

$(SDIR)/%_qt.cpp: $(SDIR)/%.h
	cd $(SDIR); moc $(notdir $<) -o $(notdir $@) -DGHOST_GUI
