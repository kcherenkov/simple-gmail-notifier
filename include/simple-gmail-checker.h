#ifndef SIMPLE_GMAIL_CHECKER_INCLUDED
#define SIMPLE_GMAIL_CHECKER_INCLUDED

static const char* FEED_URL = "https://mail.google.com/mail/feed/atom";
static const char* DOMAIN = "simple-gmail-notifier";
static const char* SERVER = "gmail.com";
static const char* PROTOCOL = "https";
static const char* TIME_TEMPLATE = "%Y-%m-%dT%H:%M:%SZ";
static const unsigned char MAX_ENTRIES = 20;

enum ResponseCode
{
    OK,
    ConnectionError,
    InvalidCredentials
};

struct feed {
    char* title;
    char* tagline;
    unsigned int fullcount;
    char* link;
    struct tm modified;
    struct entry* entries;
};

struct person {
    char* name;
    char* email;
};

struct entry {
    char* title;
    char* summary;
    char* link;
    struct tm modified;
    struct tm issued;
    char* id;
    struct person author;
    struct person* contributors;
    unsigned int contributorscount;
};

enum ResponseCode is_credentials_valid(const char* user, const char* password);

struct feed* get_feed(const char* username, const char* password);

void feed_cleanup(struct feed*);

#endif
