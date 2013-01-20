#include <curl/curl.h>
#include <mxml.h>
#include "simple-gmail-checker.h"

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = NULL;
}

// write_data is a callback function called by libcurl when it receives
// the content.  This callback function is defined by using the option
// CURLOPT_WRITEFUNCTION when calling curl_easy_setopt.
size_t write_data(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  char *temp = realloc(s->ptr, new_len+1);
  if (temp == NULL) {
    return 0;
  }
  s->ptr = temp;
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

struct feed* get_feed(const char* username, const char* password) {
    CURL* curl = curl_easy_init();
    if (curl == NULL)
    {
        return NULL;
    }

    struct string content;
    init_string(&content);

    curl_easy_setopt(curl, CURLOPT_URL, FEED_URL);
    curl_easy_setopt(curl, CURLOPT_USERNAME, username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);

    // Define callback function that will be called
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_data);

    /* passing the pointer to the response as the callback parameter */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

    // HTTP request is done here. curl calls the write_data function
    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    if (CURLE_OK != res)
        return NULL;

    mxml_node_t* tree = mxmlLoadString(NULL, content.ptr, MXML_OPAQUE_CALLBACK);

    //setvbuf(stdout, NULL, _IONBF, 0);
    //printf("%s", content);
    free(content.ptr);

    mxml_node_t* feed_node;
    feed_node = mxmlFindElement(tree, tree, "feed", NULL, NULL, MXML_DESCEND_FIRST);

    struct feed* feed = malloc(sizeof(struct feed));

    mxml_node_t* node;

    node = mxmlFindElement(feed_node, feed_node, "title", NULL, NULL, MXML_DESCEND_FIRST);
    asprintf(&feed->title, node->child->value.opaque);

    node = mxmlFindElement(node, feed_node, "tagline", NULL, NULL, MXML_NO_DESCEND);
    asprintf(&feed->tagline, node->child->value.opaque);

    node = mxmlFindElement(node, feed_node, "fullcount", NULL, NULL, MXML_NO_DESCEND);
    feed->fullcount = atoi(node->child->value.opaque);

    node = mxmlFindElement(node, feed_node, "link", "href", NULL, MXML_NO_DESCEND);
    asprintf(&feed->link, mxmlElementGetAttr(node, "href"));

    node = mxmlFindElement(node, feed_node, "modified", NULL, NULL, MXML_NO_DESCEND);
    strptime(node->child->value.opaque, TIME_TEMPLATE, &feed->modified);

    if (feed->fullcount > 0)
    {
        mxml_node_t* entry_node;
        mxml_node_t* author_node;

        feed->entries = calloc(feed->fullcount, sizeof(struct entry));

        entry_node = mxmlFindElement(node, feed_node, "entry", NULL, NULL, MXML_NO_DESCEND);

        int i;
        int count = feed->fullcount > MAX_ENTRIES ? MAX_ENTRIES - 1 : feed->fullcount - 1;
        for (i = count; i >= 0; --i)
        {
            node = mxmlFindElement(entry_node, entry_node, "title", NULL, NULL, MXML_DESCEND_FIRST);
            //if (node->child != NULL)
            asprintf(&feed->entries[i].title, node->child == NULL ? " " : node->child->value.opaque);

            node = mxmlFindElement(node, entry_node, "summary", NULL, NULL, MXML_NO_DESCEND);
            asprintf(&feed->entries[i].summary, node->child == NULL ? " " : node->child->value.opaque);

            node = mxmlFindElement(node, feed_node, "link", "href", NULL, MXML_NO_DESCEND);
            asprintf(&feed->entries[i].link, mxmlElementGetAttr(node, "href"));

            node = mxmlFindElement(node, entry_node, "modified", NULL, NULL, MXML_NO_DESCEND);
            strptime(node->child->value.opaque, TIME_TEMPLATE, &feed->entries[i].modified);

            node = mxmlFindElement(node, entry_node, "issued", NULL, NULL, MXML_NO_DESCEND);
            strptime(node->child->value.opaque, TIME_TEMPLATE, &feed->entries[i].issued);

            node = mxmlFindElement(node, entry_node, "id", NULL, NULL, MXML_NO_DESCEND);
            asprintf(&feed->entries[i].id, node->child->value.opaque);

            author_node = mxmlFindElement(node, entry_node, "author", NULL, NULL, MXML_NO_DESCEND);
            node = mxmlFindElement(author_node, author_node, "name", NULL, NULL, MXML_DESCEND_FIRST);
            asprintf(&feed->entries[i].author.name, node->child->value.opaque);

            node = mxmlFindElement(node, author_node, "email", NULL, NULL, MXML_NO_DESCEND);
            asprintf(&feed->entries[i].author.email, node->child->value.opaque);

            feed->entries[i].contributorscount = 0;
            node = mxmlFindElement(author_node, entry_node, "contributor", NULL, NULL, MXML_NO_DESCEND);
            while (node != NULL)
            {
                ++feed->entries[i].contributorscount;
                node = mxmlFindElement(node, entry_node, "contributor", NULL, NULL, MXML_NO_DESCEND);
            }

            if (feed->entries[i].contributorscount > 0)
            {
                feed->entries[i].contributors = malloc(sizeof(struct person) * feed->entries[i].contributorscount);
                mxml_node_t* contributor_node;
                contributor_node = mxmlFindElement(author_node, entry_node, "contributor", NULL, NULL, MXML_NO_DESCEND);
                int j;
                for (j = 0; j < feed->entries[i].contributorscount; ++j)
                {
                    node = mxmlFindElement(contributor_node, contributor_node, "name", NULL, NULL, MXML_DESCEND_FIRST);
                    asprintf(&feed->entries[i].contributors[j].name, node->child->value.opaque);

                    node = mxmlFindElement(node, contributor_node, "email", NULL, NULL, MXML_NO_DESCEND);
                    asprintf(&feed->entries[i].contributors[j].email, node->child->value.opaque);

                    contributor_node = mxmlFindElement(contributor_node, entry_node, "contributor", NULL, NULL, MXML_NO_DESCEND);
                }
            }

            entry_node = mxmlFindElement(entry_node, feed_node, "entry", NULL, NULL, MXML_NO_DESCEND);
        }
    }

    mxmlDelete(tree);

    return feed;
}

void feed_cleanup(struct feed* feed)
{
    // cleanup entries
    int i;
    int count = feed->fullcount > MAX_ENTRIES ? MAX_ENTRIES : feed->fullcount;
    for (i = 0; i < count; ++i)
    {
        // cleanup contributors
        int j;
        for (j = 0; j < feed->entries[i].contributorscount; ++j)
        {
            free(feed->entries[i].contributors[j].name);
            free(feed->entries[i].contributors[j].email);
        }
        if (feed->entries[i].contributorscount > 0)
            free(feed->entries[i].contributors);
        // cleanup contributors - end

        free(feed->entries[i].title);
        free(feed->entries[i].summary);
        free(feed->entries[i].link);
        free(feed->entries[i].id);
        free(feed->entries[i].author.name);
        free(feed->entries[i].author.email);
    }
    if (feed->fullcount > 0)
        free(feed->entries);
    // cleanup entries - end

    free(feed->title);
    free(feed->tagline);
    free(feed->link);
    free(feed);
}

enum ResponseCode is_credentials_valid(const char* user, const char* password)
{
    CURL* curl = curl_easy_init();
    if (curl == NULL)
    {
        return ConnectionError;
    }

    curl_easy_setopt(curl, CURLOPT_URL, FEED_URL);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);

    CURLcode res = curl_easy_perform(curl);

    long http_code;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code == 200)
        return OK;

    return InvalidCredentials;
}
