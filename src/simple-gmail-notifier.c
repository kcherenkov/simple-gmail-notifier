#include <stdlib.h>

#include <pwd.h>
#include <sys/file.h>

#include <libnotify/notify.h>
#include <libindicate/server.h>
#include <libindicate/indicator.h>
#include <libindicate-gtk/indicator.h>

#include <gnome-keyring.h>

#include "simple-gmail-checker.h"
#include "simple-gmail-notifier.h"

time_t top_message_seconds;
GMainLoop* loop;

// Parses a string, find the messages title and summary, and
// display notification to user.
int parseAndDisplayMessages(const char* username, const char* password) {

    struct feed* feed = get_feed(username, password);

    if (feed == NULL)
        return -1;

    if (feed->fullcount > 0)
    {
        notify_init (DOMAIN);
        time_t current_message_seconds;
        time_t current_top_message_seconds = 0;
        int show_msg = FALSE;
        int i;
        int count = feed->fullcount > MAX_ENTRIES ? MAX_ENTRIES : feed->fullcount;
        for (i = 0; i < count; ++i)
        {

            current_message_seconds = mktime(&feed->entries[i].modified);

            if (top_message_seconds >= current_message_seconds)
            {
                show_msg = FALSE;
                break;
            }
            else if (i == count - 1)
                current_top_message_seconds = current_message_seconds;
            show_msg = TRUE;

            if (show_msg)
            {
                NotifyNotification* notification = notify_notification_new (
                            feed->entries[i].title,
                            feed->entries[i].summary,
                            "emblem-mail");
                notify_notification_show (notification, NULL);
            }
        }


        notify_uninit();

        if (current_top_message_seconds > top_message_seconds)
            top_message_seconds = current_top_message_seconds;
    }

    int fullcount = feed->fullcount;

    feed_cleanup(feed);

    return fullcount;
}

static void
display (IndicateIndicator* indicator, gpointer data)
{
    pid_t pid = vfork();
    if (pid == 0)
    {
        execl(XDG_PATH, XDG_OPEN, GMAIL_URL, NULL);
        _exit(EXIT_FAILURE);  /* terminate the child in case execl fails */
    }
}

static void
server_display (IndicateServer* server, gpointer data)
{
    pid_t pid = vfork();
    if (pid == 0)
    {
        execl(CONFIG_APP, CONFIG_APP, NULL);
        _exit(EXIT_FAILURE);  /* terminate the child in case execl fails */
    }
}

static GnomeKeyringOperationGetListCallback
show_config_window (GnomeKeyringResult res, GList* list, gpointer data)
{
    if (res != GNOME_KEYRING_RESULT_OK)
    {
        //show config window
        server_display(NULL, NULL);
    }
}

static GnomeKeyringOperationGetListCallback
found_password (GnomeKeyringResult res, GList* list, gpointer data)
{
    /* user_data will be the same as was passed to gnome_keyring_find_password() */

    GnomeKeyringNetworkPasswordData* firstAccount = (GnomeKeyringNetworkPasswordData*)g_list_nth_data(list, 0);
    IndicateIndicator* indicator = INDICATE_INDICATOR(data);
    if (res == GNOME_KEYRING_RESULT_OK)
    {
            int count = parseAndDisplayMessages(firstAccount->user, firstAccount->password);
            indicate_indicator_set_property(INDICATE_INDICATOR(indicator), "sender", count != -1 ? firstAccount->user : "Connection error");
            if (!indicate_indicator_is_visible (INDICATE_INDICATOR(indicator)))
                indicate_indicator_show(INDICATE_INDICATOR(indicator));
            if (count > 0)
            {
                char* c = NULL;
                asprintf(&c, "%d", count);
                indicate_indicator_set_property(INDICATE_INDICATOR(indicator), INDICATOR_COUNT, c);
                indicate_indicator_set_property_bool(INDICATE_INDICATOR(indicator), DRAW_ATTENTION, TRUE);
                free(c);
            }
            else
            {
                indicate_indicator_set_property(INDICATE_INDICATOR(indicator), INDICATOR_COUNT, count == 0 ? "0" : "");
                indicate_indicator_set_property_bool(INDICATE_INDICATOR(indicator), DRAW_ATTENTION, FALSE);
            }
    }
    /* Once this function returns |password| will be freed */
}

static gboolean
timeout_cb (gpointer data)
{
    gnome_keyring_find_network_password (NULL,                  //user
                                         DOMAIN,                //domain
                                         SERVER,                //server
                                         NULL,                  //object
                                         PROTOCOL,              //protocol
                                         NULL,                  //authtype
                                         0,                     //port
                                         (GnomeKeyringOperationGetListCallback)&found_password,       //callback
                                         data,
                                         NULL);
    return TRUE;
}

static void
onSIGCHLD(int sig)
{
    wait3(0, 0, NULL);
}

static void
quit (int sig)
{
    g_main_loop_quit(loop);
}

/*
static void
quit (IndicateIndicator* indicator, gpointer data)
{
    g_main_loop_quit(loop);
}
*/

int main(int argc, char *argv[]) {
    char* lockfile = getpwuid(getuid())->pw_dir;
    asprintf(&lockfile, "%s/%s", lockfile, ".simple-gmail-notifier.lock");
    FILE* f = fopen(lockfile, "w");
    int result = flock(fileno(f), LOCK_EX | LOCK_NB);
    if (result != 0)
    {
        fclose(f);
        free(lockfile);
        return EXIT_SUCCESS;
    }
    g_type_init();
    signal(SIGCHLD, &onSIGCHLD);
    signal(SIGTERM, &quit);

    IndicateServer* server = indicate_server_ref_default();
    indicate_server_set_type(server, "message.mail");
    indicate_server_set_desktop_file(server, DESKTOP_FILE);
    g_signal_connect(G_OBJECT(server), INDICATE_SERVER_SIGNAL_SERVER_DISPLAY, G_CALLBACK(server_display), NULL);

    IndicateIndicator* indicator;
    indicator = indicate_indicator_new();
    indicate_indicator_set_property(INDICATE_INDICATOR(indicator), "subtype", "mail");

    g_signal_connect(G_OBJECT(indicator), INDICATE_INDICATOR_SIGNAL_DISPLAY, G_CALLBACK(display), NULL);

    //First run check for credentials
    gnome_keyring_find_network_password (NULL,                  //user
                                         DOMAIN,                //domain
                                         SERVER,                //server
                                         NULL,                  //object
                                         PROTOCOL,              //protocol
                                         NULL,                  //authtype
                                         0,                     //port
                                         (GnomeKeyringOperationGetListCallback)&show_config_window,   //callback
                                         NULL,
                                         NULL);

    timeout_cb(indicator);
    g_timeout_add_seconds(TIMEOUT_SECONDS, timeout_cb, indicator);

    loop = g_main_loop_new(NULL, FALSE);

    indicator = indicate_indicator_new();
    indicate_indicator_set_property(INDICATE_INDICATOR(indicator), "sender", "Exit");
    g_signal_connect(G_OBJECT(indicator), INDICATE_INDICATOR_SIGNAL_DISPLAY, G_CALLBACK(quit), NULL);
    indicate_indicator_show(INDICATE_INDICATOR(indicator));

    g_main_loop_run(loop);
    g_object_unref(loop);

    flock(fileno(f), LOCK_UN);
    fclose(f);
    remove(lockfile);
    free(lockfile);

    return EXIT_SUCCESS;
}
