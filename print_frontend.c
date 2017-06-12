#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include "frontend_interface.h"
#include "backend_interface.h"
#include "print_data_structures.h"

#define DIALOG_BUS_NAME "org.openprinting.PrintFrontend"
#define DIALOG_OBJ_PATH "/"
static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data);
static void on_printer_added(GDBusConnection *connection,
                             const gchar *sender_name,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *signal_name,
                             GVariant *parameters,
                             gpointer user_data);
static void on_printer_removed(GDBusConnection *connection,
                               const gchar *sender_name,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *signal_name,
                               GVariant *parameters,
                               gpointer user_data);
gpointer parse_commands(gpointer user_data);
FrontendObj *f;
int main()
{
   // printers = g_hash_table_new(g_str_hash, g_str_equal);
    //backends = g_hash_table_new(g_str_hash, g_str_equal);
    
    f = get_new_FrontendObj();
    g_bus_own_name(G_BUS_TYPE_SESSION,
                   DIALOG_BUS_NAME,
                   0,                //flags
                   NULL,             //bus_acquired_handler
                   on_name_acquired, //name acquired handler
                   NULL,             //name_lost handler
                   f,                //user_data
                   NULL);            //user_data free function
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
}

static void
on_name_acquired(GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
    PrintFrontend *skeleton;
    skeleton = print_frontend_skeleton_new();
    GError *error = NULL;

    g_dbus_connection_signal_subscribe(connection,
                                       NULL,                            //Sender name
                                       "org.openprinting.PrintBackend", //Sender interface
                                       PRINTER_ADDED_SIGNAL,            //Signal name
                                       NULL,                            /**match on all object paths**/
                                       NULL,                            /**match on all arguments**/
                                       0,                               //Flags
                                       on_printer_added,                //callback
                                       user_data,                       //user_data
                                       NULL);

    g_dbus_connection_signal_subscribe(connection,
                                       NULL,                            //Sender name
                                       "org.openprinting.PrintBackend", //Sender interface
                                       PRINTER_REMOVED_SIGNAL,          //Signal name
                                       NULL,                            /**match on all object paths**/
                                       NULL,                            /**match on all arguments**/
                                       0,                               //Flags
                                       on_printer_removed,              //callback
                                       user_data,                       //user_data
                                       NULL);

    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton), connection, DIALOG_OBJ_PATH, &error);
    g_assert_no_error(error);

    print_frontend_emit_get_backend(skeleton);

    /**
    I have created the following thread just for testing purpose.
    In reality you don't need a separate thread to parse commands because you already have a GUI. 
    **/
    g_thread_new("parse_commands_thread", parse_commands, skeleton);
}
static void on_printer_added(GDBusConnection *connection,
                             const gchar *sender_name,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *signal_name,
                             GVariant *parameters,
                             gpointer user_data)
{

    PrinterObj *p = get_new_PrinterObj();
    fill_basic_properties(p, parameters);
   // g_variant_get(parameters, "(sssss)", &printer_name, &info, &location, &make_and_model, &is_accepting_jobs);
    add_printer(f,p);
    print_basic_properties(p);
    
}
static void on_printer_removed(GDBusConnection *connection,
                               const gchar *sender_name,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *signal_name,
                               GVariant *parameters,
                               gpointer user_data)
{
    char *printer_name;
    g_variant_get(parameters, "(s)", &printer_name);
    g_message("Removed Printer %s!\n", printer_name);
}

gpointer parse_commands(gpointer user_data)
{
    PrintFrontend *skeleton = (PrintFrontend *)user_data;
    char buf[100];
    while (1)
    {
        scanf("%s", buf);
        if (strcmp(buf, "stop") == 0)
        {
            print_frontend_emit_stop_listing(skeleton);
            g_message("Stopping front end..\n");
            exit(0);
        }
        else if (strcmp(buf, "refresh") == 0)
        {
            print_frontend_emit_refresh_backend(skeleton);
            g_message("Sending refresh request..\n");
        }
    }
}