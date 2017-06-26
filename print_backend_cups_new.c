#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <cups/cups.h>
#include "backend_interface.h"
#include "common_helper.h"
#include "backend_helper.h"

#define _CUPS_NO_DEPRECATED 1
#define BUS_NAME "org.openprinting.Backend.CUPS"
#define OBJECT_PATH "/"

static void acquire_session_bus_name();
static void on_name_acquired(GDBusConnection *connection,
                             const gchar *name,
                             gpointer not_used);
static gboolean on_handle_activate_backend(PrintBackend *interface,
                                           GDBusMethodInvocation *invocation,
                                           gpointer not_used);
gpointer list_printers(gpointer _dialog_name);
int send_printer_added(void *_dialog_name, unsigned flags, cups_dest_t *dest);
static void on_stop_backend(GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer not_used);
void connect_to_signals();

BackendObj *b;
Mappings *m;
int main()
{
    b = get_new_BackendObj();
    m = get_new_Mappings();

    acquire_session_bus_name(BUS_NAME);
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
}

static void acquire_session_bus_name(char *bus_name)
{
    g_bus_own_name(G_BUS_TYPE_SESSION,
                   bus_name,
                   0,                //flags
                   NULL,             //bus_acquired_handler
                   on_name_acquired, //name acquired handler
                   NULL,             //name_lost handler
                   NULL,             //user_data
                   NULL);            //user_data free function
}

static void
on_name_acquired(GDBusConnection *connection,
                 const gchar *name,
                 gpointer not_used)
{
    b->dbus_connection = connection;
    b->skeleton = print_backend_skeleton_new();
    connect_to_signals();
    connect_to_dbus(b, OBJECT_PATH);
}
static gboolean on_handle_activate_backend(PrintBackend *interface,
                                           GDBusMethodInvocation *invocation,
                                           gpointer not_used)
{
    /**
    This function starts the backend and starts sending the printers
    **/
    const char *dialog_name = g_dbus_method_invocation_get_sender(invocation); /// potential risk
    add_frontend(b, dialog_name);
    g_thread_new(NULL, list_printers, (gpointer)dialog_name);
    return TRUE;
}

gpointer list_printers(gpointer _dialog_name)
{
    char *dialog_name = (char *)_dialog_name;
    g_message("New thread for dialog at %s\n", dialog_name);
    int *cancel = get_dialog_cancel(b, dialog_name);

    cupsEnumDests(CUPS_DEST_FLAGS_NONE,
                  -1, //NO timeout
                  cancel,
                  0,    //TYPE
                  0,    //MASK
                  send_printer_added, 
                  _dialog_name);

    g_message("Exiting thread for dialog at %s\n", dialog_name);
}
int send_printer_added(void *_dialog_name, unsigned flags, cups_dest_t *dest)
{

    char *dialog_name = (char *)_dialog_name;
    char *printer_name = dest->name;

    if (dialog_contains_printer(b,dialog_name,printer_name))
    {
        g_message("%s already sent.\n", printer_name);
        return 1;
    }

    add_printer_to_dialog(b,dialog_name,printer_name);
    send_printer_added_signal(b,dialog_name,dest);
    g_message("     Sent notification for printer %s\n", printer_name);

    ///fix memory leaks
    return 1; //continue enumeration
}
static void on_stop_backend(GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer not_used)
{
     g_message("Stop backend signal from %s\n", sender_name);
     set_dialog_cancel(b,sender_name);
     remove_frontend(b,sender_name);
     if(no_frontends(b))
     {
         g_message("No frontends connected .. exiting backend.\n");
         exit(EXIT_SUCCESS);
     }
}


void connect_to_signals()
{
    PrintBackend *skeleton = b->skeleton;
    g_signal_connect(skeleton,                               //instance
                     "handle-activate-backend",              //signal name
                     G_CALLBACK(on_handle_activate_backend), //callback
                     NULL);

    // g_signal_connect(skeleton,                                 //instance
    //                  "handle-list-basic-options",              //signal name
    //                  G_CALLBACK(on_handle_list_basic_options), //callback
    //                  NULL);                                    //user_data

    // g_signal_connect(skeleton,                                       //instance
    //                  "handle-get-printer-capabilities",              //signal name
    //                  G_CALLBACK(on_handle_get_printer_capabilities), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                //instance
    //                  "handle-get-default-value",              //signal name
    //                  G_CALLBACK(on_handle_get_default_value), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                   //instance
    //                  "handle-get-supported-values-raw-string",   //signal name
    //                  G_CALLBACK(on_handle_get_supported_values), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                  //instance
    //                  "handle-get-supported-media",              //signal name
    //                  G_CALLBACK(on_handle_get_supported_media), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                  //instance
    //                  "handle-get-supported-color",              //signal name
    //                  G_CALLBACK(on_handle_get_supported_color), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                    //instance
    //                  "handle-get-supported-quality",              //signal name
    //                  G_CALLBACK(on_handle_get_supported_quality), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                        //instance
    //                  "handle-get-supported-orientation",              //signal name
    //                  G_CALLBACK(on_handle_get_supported_orientation), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                //instance
    //                  "handle-get-printer-state",              //signal name
    //                  G_CALLBACK(on_handle_get_printer_state), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                                //instance
    //                  "handle-is-accepting-jobs",              //signal name
    //                  G_CALLBACK(on_handle_is_accepting_jobs), //callback
    //                  NULL);
    // g_signal_connect(skeleton,                             //instance
    //                  "handle-get-resolution",              //signal name
    //                  G_CALLBACK(on_handle_get_resolution), //callback
    //                  NULL);
    // /**subscribe to signals **/
    
    g_dbus_connection_signal_subscribe(b->dbus_connection,
                                       NULL,                             //Sender name
                                       "org.openprinting.PrintFrontend", //Sender interface
                                       STOP_BACKEND_SIGNAL,              //Signal name
                                       NULL,                             /**match on all object paths**/
                                       NULL,                             /**match on all arguments**/
                                       0,                                //Flags
                                       on_stop_backend,                  //callback
                                       NULL,                             //user_data
                                       NULL);
    // g_dbus_connection_signal_subscribe(dbus_connection,
    //                                    NULL,                             //Sender name
    //                                    "org.openprinting.PrintFrontend", //Sender interface
    //                                    REFRESH_BACKEND_SIGNAL,           //Signal name
    //                                    NULL,                             /**match on all object paths**/
    //                                    NULL,                             /**match on all arguments**/
    //                                    0,                                //Flags
    //                                    on_refresh_backend,               //callback
    //                                    NULL,                             //user_data
    //                                    NULL);
}