#include <stdlib.h>
#include <gtk/gtk.h>

#include <gnome-keyring.h>

#include <pwd.h>
#include <sys/stat.h>

#include "simple-gmail-checker.h"
#include "simple-gmail-notifier.h"

const char* homedir;

typedef struct {
    GtkWidget* input_user;
    GtkWidget* input_password;
    GtkWidget* label_credentials;
    GtkWidget* image_credentials;
    GtkWidget* button_ok;
    GtkWidget* is_autoconnect;
} widgets;

static GnomeKeyringOperationGetIntCallback
stored_password (GnomeKeyringResult res, guint32 val, widgets *data)
{
    /* user_data will be the same as was passed to gnome_keyring_store_password() */

    if (res != GNOME_KEYRING_RESULT_OK)
    {
        gtk_image_set_from_stock(GTK_IMAGE(data->image_credentials), "gtk-stop", GTK_ICON_SIZE_BUTTON);
        gtk_label_set_label(GTK_LABEL(data->label_credentials), gnome_keyring_result_to_message (res));
    }
}

static GnomeKeyringOperationGetListCallback
found_password (GnomeKeyringResult res, GList* list, widgets* data)
{
    /* user_data will be the same as was passed to gnome_keyring_find_password() */

    if (res == GNOME_KEYRING_RESULT_OK)
    {
        GnomeKeyringNetworkPasswordData* firstAccount = (GnomeKeyringNetworkPasswordData*)g_list_nth_data(list, 0);
        gtk_entry_set_text(GTK_ENTRY(data->input_user), firstAccount->user);
        gtk_entry_set_text(GTK_ENTRY(data->input_password), firstAccount->password);
    }

    /* Once this function returns |password| will be freed */
}

static GThreadFunc check_credentials (widgets* data)
{
    const char* user = gtk_entry_get_text(GTK_ENTRY(data->input_user));
    const char* password = gtk_entry_get_text(GTK_ENTRY(data->input_password));

    switch (is_credentials_valid(user, password))
    {
    case OK:
        gtk_image_set_from_stock(GTK_IMAGE(data->image_credentials), "gtk-yes", GTK_ICON_SIZE_BUTTON);
        gtk_label_set_label(GTK_LABEL(data->label_credentials), "All right");

        gnome_keyring_set_network_password  (NULL,                    //keyring
                                             user,                    //user
                                             DOMAIN,                  //domain
                                             SERVER,                  //server
                                             NULL,                    //object
                                             PROTOCOL,                //protocol
                                             NULL,                    //authtype
                                             0,                       //port
                                             password,                //password
                                             (GnomeKeyringOperationGetIntCallback)&stored_password,         //callback
                                             data,
                                             NULL);
        break;
    case InvalidCredentials:
        gtk_image_set_from_stock(GTK_IMAGE(data->image_credentials), "gtk-stop", GTK_ICON_SIZE_BUTTON);
        gtk_label_set_label(GTK_LABEL(data->label_credentials), "Invalid credentials");
        break;
    case ConnectionError:
        gtk_image_set_from_stock(GTK_IMAGE(data->image_credentials), "gtk-stop", GTK_ICON_SIZE_BUTTON);
        gtk_label_set_label(GTK_LABEL(data->label_credentials), "Connection error");
        break;
    }

    gtk_widget_set_sensitive(data->button_ok, TRUE);
    gtk_widget_set_sensitive(data->input_user, TRUE);
    gtk_widget_set_sensitive(data->input_password, TRUE);
}

void
ok_button_clicked (GtkObject *object, widgets* data)
{
    const char* user = gtk_entry_get_text(GTK_ENTRY(data->input_user));
    const char* password = gtk_entry_get_text(GTK_ENTRY(data->input_password));

    if (user[0] == 0 || password[0] == 0)
        return;

    gtk_image_set_from_stock(GTK_IMAGE(data->image_credentials), "gtk-refresh", GTK_ICON_SIZE_BUTTON);
    gtk_label_set_label(GTK_LABEL(data->label_credentials), "Checking...");
    gtk_widget_set_sensitive(data->button_ok, FALSE);
    gtk_widget_set_sensitive(data->input_user, FALSE);
    gtk_widget_set_sensitive(data->input_password, FALSE);

    if (!g_thread_create((GThreadFunc)&check_credentials, data, FALSE, NULL) != 0)
        g_warning("can't create the thread");
}

void
is_autoconnect_toggled (GtkObject *object, widgets* data)
{
    gboolean is_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->is_autoconnect));
    char* link_path = NULL;
    asprintf(&link_path, "%s/%s", homedir, AUTOSTART_DESKTOP_FILE);
    if (is_active)
    {
        if (symlink(DESKTOP_FILE, link_path) != 0)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->is_autoconnect), FALSE);
    }
    else
    {
        if (unlink(link_path) != 0)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->is_autoconnect), TRUE);
    }
    free(link_path);
}


int main(int argc, char *argv[]) {
    gtk_init(NULL, NULL);
    GtkBuilder* builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, UI_FILE, NULL);

    widgets data;
    data.input_user = GTK_WIDGET (gtk_builder_get_object (builder, "input_user"));
    data.input_password = GTK_WIDGET (gtk_builder_get_object (builder, "input_password"));
    data.label_credentials = GTK_WIDGET (gtk_builder_get_object (builder, "label_credentials"));
    data.image_credentials = GTK_WIDGET (gtk_builder_get_object (builder, "image_credentials"));
    data.button_ok = GTK_WIDGET (gtk_builder_get_object (builder, "button_ok"));
    data.is_autoconnect = GTK_WIDGET (gtk_builder_get_object (builder, "is_autoconnect"));

    homedir = getpwuid(getuid())->pw_dir;

    char* link_path = NULL;
    asprintf(&link_path, "%s/%s", homedir, AUTOSTART_DESKTOP_FILE);

    struct stat buf;
    if (stat(link_path, &buf) == 0)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data.is_autoconnect), TRUE);
    free(link_path);

    gnome_keyring_find_network_password (NULL,                  //user
                                         DOMAIN,                //domain
                                         SERVER,                //server
                                         NULL,                  //object
                                         PROTOCOL,              //protocol
                                         NULL,                  //authtype
                                         0,                     //port
                                         (GnomeKeyringOperationGetListCallback)&found_password,       //callback
                                         &data,
                                         NULL);

    gtk_builder_connect_signals (builder, &data);
    GtkWidget* dialog = GTK_WIDGET (gtk_builder_get_object (builder, "dialog"));

    g_object_unref (G_OBJECT (builder));

    gtk_widget_show_all(dialog);

    gtk_main ();

    return EXIT_SUCCESS;
}
