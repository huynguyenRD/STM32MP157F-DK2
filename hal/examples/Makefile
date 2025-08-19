CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
LDFLAGS = 
TARGET = evdev_min
SRCDIR = .
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = evdev_min.c
OBJECTS = $(OBJDIR)/evdev_min.o

# Create directories if they don't exist
$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Default target
all: $(BINDIR)/$(TARGET)

# Build the executable
$(BINDIR)/$(TARGET): $(OBJECTS)
    $(CC) $(OBJECTS) -o $@ $(LDFLAGS)
    @echo "Built: $@"

# Compile source files
$(OBJDIR)/%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@
    @echo "Compiled: $<"

# Clean build files
clean:
    rm -rf $(OBJDIR) $(BINDIR)
    @echo "Cleaned build files"

# Install (copy to /usr/local/bin)
install: $(BINDIR)/$(TARGET)
    sudo cp $(BINDIR)/$(TARGET) /usr/local/bin/
    @echo "Installed to /usr/local/bin/"

# Remove from system
uninstall:
    sudo rm -f /usr/local/bin/$(TARGET)
    @echo "Uninstalled from /usr/local/bin/"

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: all

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: all

# Run the program
run: $(BINDIR)/$(TARGET)
    ./$(BINDIR)/$(TARGET)

# Show help
help:
    @echo "Available targets:"
    @echo "  all      - Build the program (default)"
    @echo "  clean    - Remove build files"
    @echo "  debug    - Build with debug symbols"
    @echo "  release  - Build optimized release version"
    @echo "  install  - Install to /usr/local/bin"
    @echo "  uninstall- Remove from /usr/local/bin"
    @echo "  run      - Build and run the program"
    @echo "  help     - Show this help"

.PHONY: all clean install uninstall debug release run help