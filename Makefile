.PHONY: all clean

CXX=g++
SDIR=GhostServer
ODIR=obj

# If this is the wrong version, try /usr/lib/qt6/uic
UIC=uic
MOC=moc
RCC=rcc

SRCS_GUI=$(SDIR)/main.cpp $(SDIR)/mainwindow.cpp $(SDIR)/commands.cpp $(SDIR)/networkmanager.cpp $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp $(SDIR)/GhostServer_qrc.cpp
OBJS_GUI=$(patsubst $(SDIR)/%.cpp, $(ODIR)/gui/%.o, $(SRCS_GUI))

SRCS_CLI=$(SDIR)/main_cli.cpp $(SDIR)/commands.cpp $(SDIR)/networkmanager.cpp
OBJS_CLI=$(patsubst $(SDIR)/%.cpp, $(ODIR)/cli/%.o, $(SRCS_CLI))

DEPS=$(OBJS_GUI:%.o=%.d)

CXXFLAGS=-std=c++17 -fPIC -Ilib/SFML/include -DSFML_STATIC
CXXFLAGS_GUI=-DGHOST_GUI $(shell pkg-config --cflags Qt6Widgets)
CXXFLAGS_CLI=

LDFLAGS=-lpthread -Llib/SFML/lib/linux -lsfml-network -lsfml-system
LDFLAGS_GUI=$(shell pkg-config --libs Qt6Widgets)
LDFLAGS_CLI=

-include config.mk

all: ghost_server ghost_server_cli
clean:
	rm -rf $(ODIR) ghost_server ghost_server_cli
	rm -rf $(SDIR)/ui_mainwindow.h $(SDIR)/mainwindow_qt.cpp $(SDIR)/networkmanager_qt.cpp $(SDIR)/GhostServer_qrc.cpp

-include $(DEPS)

ghost_server: $(SDIR)/ui_mainwindow.h $(OBJS_GUI)
	$(CXX) $(OBJS_GUI) $(LDFLAGS) $(LDFLAGS_GUI) -o $@

ghost_server_cli: $(OBJS_CLI)
	$(CXX) $(OBJS_CLI) $(LDFLAGS) $(LDFLAGS_CLI) -o $@

$(ODIR)/gui/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_GUI) -MMD -c $< -o $@

$(ODIR)/cli/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_CLI) -MMD -c $< -o $@

$(SDIR)/ui_mainwindow.h: $(SDIR)/mainwindow.ui
	$(UIC) $< -o $@

$(SDIR)/%_qt.cpp: $(SDIR)/%.h
	cd $(SDIR); $(MOC) $(notdir $<) -o $(notdir $@) -DGHOST_GUI

$(SDIR)/GhostServer_qrc.cpp: $(SDIR)/GhostServer.qrc
	$(RCC) $< -o $@
