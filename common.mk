CC      ?= cc
# CFLAGS   = -std=c99 -Wall -Wextra -O0 -g # debug
CFLAGS   = -std=c99 -Wall -Wextra -O2
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude

OUT      = neuwm
SRC      = source/neuwm.c source/util.c

PKGS = swc wayland-server xkbcommon libinput pixman-1 libdrm wld libudev xcb xcb-composite xcb-ewmh xcb-icccm

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(OUT) $(SRC) $(LDLIBS)

clean:
	rm -f $(OUT) $(SNAP_OUT)

install: all
	install -Dm755 $(OUT) /usr/local/bin/$(OUT)

compile_flags:
	rm -f compile_flags.txt
	for f in $(CFLAGS) $(CPPFLAGS); do echo $$f >> compile_flags.txt; done
