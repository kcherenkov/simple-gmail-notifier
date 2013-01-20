#ifndef SIMPLE_GMAIL_NOTIFIER_INCLUDED
#define SIMPLE_GMAIL_NOTIFIER_INCLUDED

static const uint TIMEOUT_SECONDS = 60;

static const char* CONFIG_APP = "/usr/bin/simple-gmail-notifier-config";
static const char* UI_FILE = "/usr/share/simple-gmail-notifier/config.glade";
static const char* DESKTOP_FILE = "/usr/share/applications/simple-gmail-notifier.desktop";
static const char* AUTOSTART_DESKTOP_FILE = ".config/autostart/simple-gmail-notifier.desktop";

static const char* XDG_PATH = "/usr/bin/xdg-open";
static const char* XDG_OPEN = "xdg-open";
static const char* GMAIL_URL = "http://gmail.com";

static const char* INDICATOR_COUNT = "count";
static const char* DRAW_ATTENTION = "draw-attention";

#endif
