#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-rr.h>
#include <libgnome-desktop/gnome-rr-config.h>

static GnomeRRRotation
string_to_rotation(const gchar *str)
{
    typedef struct {
        const gchar *str;
        GnomeRRRotation rot;
    } str_rot_pair;

    static const str_rot_pair const str_rot_pairs[] = {
        { "left",      GNOME_RR_ROTATION_90 },
        { "left-up",   GNOME_RR_ROTATION_90 },
        { "right",     GNOME_RR_ROTATION_270 },
        { "right-up",  GNOME_RR_ROTATION_270 },
        { "flip",      GNOME_RR_ROTATION_180 },
        { "bottom",    GNOME_RR_ROTATION_180 },
        { "bottom-up", GNOME_RR_ROTATION_180 },
        { "up",        GNOME_RR_ROTATION_0 },
        { "top-up",    GNOME_RR_ROTATION_0 },
        { "reset",     GNOME_RR_ROTATION_0 },
    };

    for(size_t i = 0; i < sizeof(str_rot_pairs) / sizeof(str_rot_pairs[0]); ++i) {
        const str_rot_pair* const pair = &str_rot_pairs[i];

        if(g_ascii_strcasecmp(pair->str, str) == 0)
            return pair->rot;
    }

    return 0;
}

static const gchar*
rotation_to_pixel_order(GnomeRRRotation rot)
{
    switch(rot) {
    case GNOME_RR_ROTATION_0:
        return "rgb";
        break;
    case GNOME_RR_ROTATION_90:
        return "vrgb";
        break;
    case GNOME_RR_ROTATION_180:
        return "bgr";
        break;
    case GNOME_RR_ROTATION_270:
        return "vbgr";
        break;
    default:
        return NULL;
    }
}

static GnomeRRRotation
rotation_flip(GnomeRRRotation rot)
{
    switch(rot) {
    case GNOME_RR_ROTATION_0:
        return GNOME_RR_ROTATION_90;
        break;
    case GNOME_RR_ROTATION_90:
        return GNOME_RR_ROTATION_0;
        break;
    case GNOME_RR_ROTATION_180:
        return GNOME_RR_ROTATION_270;
        break;
    case GNOME_RR_ROTATION_270:
        return GNOME_RR_ROTATION_180;
        break;
    default:
        return GNOME_RR_ROTATION_0;
    }
}

static gboolean list_outputs = FALSE;
static gboolean list_output_infos = FALSE;
static gchar *orientation = NULL;
static GnomeRRRotation orientation_rot = 0;
static gint output_id = -1;
static gchar *output_name = NULL;

static const GOptionEntry const entries[] = {
    {"list-outputs",     'l', 0, G_OPTION_ARG_NONE,   &list_outputs,      "List existing outputs, perform no action", NULL},
    {"list-output-infos", 0,  0, G_OPTION_ARG_NONE,   &list_output_infos, "List existing outputs infos, perform no action", NULL},
    {"orientation",      'o', 0, G_OPTION_ARG_STRING, &orientation,       "Manual display orientation", NULL},
    {"output-id",        'i', 0, G_OPTION_ARG_INT,    &output_id,         "Screen output id", NULL},
    {"output-name",      'n', 0, G_OPTION_ARG_STRING, &output_name,       "Screen output name", NULL},
    {NULL}
};

static void
print_help(GOptionContext *context)
{
    g_assert(context != NULL);

    gchar * const help_content  = g_option_context_get_help(context, TRUE, NULL);
    g_assert(help_content != NULL);

    g_print("%s", help_content);

    g_free(help_content);
}

static void
parse_args(int *argc, char ***argv)
{
    GOptionContext * const context  = g_option_context_new(NULL);
    g_assert(context != NULL);

    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gtk_get_option_group(FALSE));
    g_option_context_set_help_enabled(context, TRUE);

    GError *error = NULL;
    if(!g_option_context_parse(context, argc, argv, &error)) {
        if(error != NULL) {
            g_warning("%s", error->message);
            g_error_free(error);
        } else {
            g_warning("Unable to initialize GTK+");
        }

        exit(EXIT_FAILURE);
    }

    if(list_outputs) {
        // pass
    } else if(list_output_infos) {
        // pass
    } else {
        if(output_id >= 0 && output_name != NULL) {
            g_warning("Only one of 'output-id' and 'output-name' must be specified.\n");
            print_help(context);
            exit(EXIT_FAILURE);
        }

        if(orientation != NULL) {
            orientation_rot = string_to_rotation(orientation);

            if(orientation_rot == 0) {
                g_warning("Invalid orientation.\n");
                print_help(context);
                exit(EXIT_FAILURE);
            }
        }
    }

    g_option_context_free(context);
}

static GnomeRRScreen
*get_default_screen()
{
    GError *error  = NULL;
    GnomeRRScreen * const screen = gnome_rr_screen_new(gdk_screen_get_default(), &error);

    g_assert_no_error(error);
    g_assert(screen != NULL);

    return screen;
}

static GnomeRROutput*
get_output_default(GnomeRRScreen *screen)
{
    g_assert(screen != NULL);

    GnomeRROutput ** const outputs = gnome_rr_screen_list_outputs(screen);
    g_assert(outputs != NULL);

    GnomeRROutput *result = NULL;
    for(int i = 0; outputs[i] != NULL; ++i) {
        GnomeRROutput * const output = outputs[i];
        if(gnome_rr_output_get_is_primary(output)) {
            result = output;
            break;
        }
    }

    g_assert(result != NULL);
    return result;
}

static GnomeRROutput*
get_output_by_name(GnomeRRScreen *screen, const gchar *output_name)
{
    g_assert(screen != NULL);
    g_assert(output_name != NULL);

    GnomeRROutput * const output = gnome_rr_screen_get_output_by_name(screen, output_name);
    g_assert(output != NULL);

    return output;
}

static GnomeRROutput*
get_output_by_id(GnomeRRScreen *screen, gint id)
{

    g_assert(screen != NULL);
    g_assert(id >= 0);

    GnomeRROutput * const output = gnome_rr_screen_get_output_by_id(screen, (guint32)id);
    g_assert(output != NULL);

    return output;
}

static GnomeRRConfig*
get_current_config(GnomeRRScreen *screen)
{

    g_assert(screen != NULL);

    GError *error = NULL;
    GnomeRRConfig * const config = gnome_rr_config_new_current(screen, &error);
    g_assert(config != NULL);

    return config;
}

static GnomeRROutputInfo*
get_output_info_by_name(GnomeRRConfig *config, const gchar *name)
{
    g_assert(config != NULL);
    g_assert(name != NULL);

    GnomeRROutputInfo **infos = gnome_rr_config_get_outputs(config);
    g_assert(infos != NULL);

    GnomeRROutputInfo *result = NULL;
    for(int i = 0; infos[i] != NULL; ++i) {
        GnomeRROutputInfo * const info = infos[i];
        const gchar *info_name = gnome_rr_output_info_get_name(info);

        if(g_ascii_strcasecmp(name, info_name) == 0) {
            result = info;
            break;
        }
    }

    g_assert(result != NULL);
    return result;
}

static GnomeRRRotation
output_rotation_flip(GnomeRROutput* output)
{
    g_assert(output != NULL);

    GnomeRRCrtc *crtc = gnome_rr_output_get_crtc(output);
    g_assert(crtc != NULL);

    GnomeRRRotation current = gnome_rr_crtc_get_current_rotation(crtc);
    return rotation_flip(current);
}

static gboolean
set_pixel_order(const gchar *order)
{
    g_assert(order != NULL);

    GSettings * const settings = g_settings_new("org.gnome.settings-daemon.plugins.xsettings");
    g_assert(settings != NULL);

    gboolean result = g_settings_set_string(settings, "rgba-order", order);

    g_clear_object(&settings);

    return result;
}

static int
do_list_outputs(GnomeRRScreen *screen)
{
    GnomeRROutput ** const outputs = gnome_rr_screen_list_outputs(screen);
    g_assert(outputs != NULL);

    for(int i = 0; outputs[i] != NULL; ++i) {
        GnomeRROutput * const output = outputs[i];
        const int id = gnome_rr_output_get_id(output);
        const gchar * const name = gnome_rr_output_get_name(output);

        g_print("%d: %s\n", id, name);
        //g_free(name);
    }

    return 0;
}

static int
do_list_output_infos(GnomeRRScreen *screen)
{
    GError *error = NULL;
    GnomeRRConfig * const config = gnome_rr_config_new_current(screen, &error);
    g_assert_no_error(error);
    g_assert(config != NULL);

    GnomeRROutputInfo ** const infos = gnome_rr_config_get_outputs(config);
    g_assert(infos != NULL);

    for(int i = 0; infos[i] != NULL; ++i) {
        GnomeRROutputInfo * const output_info = infos[i];
        const gchar* const name = gnome_rr_output_info_get_name(output_info);
        const gchar* const display_name = gnome_rr_output_info_get_name(output_info);

        g_print("name: %s, display-name: %s\n", name, display_name);
    }

    return 0;
}

int main(int argc, char **argv)
{
    int result;
    GError *error;

    gdk_init(&argc, &argv);
    parse_args(&argc, &argv);

    g_debug("Acquiring default screen.");

    GnomeRRScreen * const screen = get_default_screen();

    if(list_outputs) {
        result = do_list_outputs(screen);
        goto cleanup;
    }

    if(list_output_infos) {
        result = do_list_output_infos(screen);
        goto cleanup;
    }


    g_debug("Retrieving display output.");

    GnomeRROutput *output;
    if(output_id >= 0)
        output = get_output_by_id(screen, output_id);
    else if(output_name != NULL)
        output = get_output_by_name(screen, output_name);
    else
        output = get_output_default(screen);


    g_debug("Retrieving output configuration.");

    GnomeRRConfig * const config = get_current_config(screen);
    GnomeRROutputInfo * const output_info = get_output_info_by_name(config, gnome_rr_output_get_name(output));


    g_debug("Applying display configuration.");

    if(orientation == NULL) {
        orientation_rot = output_rotation_flip(output);
        g_debug("auto flip: %d\n", orientation_rot);
    } else {
        g_debug("orientation: %s %d\n", orientation, orientation_rot);
    }

    gnome_rr_output_info_set_rotation(output_info, orientation_rot);
        
    error = NULL;
    
#if HAVE_GNOME_RR_CONFIG_APPLY
    gnome_rr_config_apply(config, screen, &error);
#elif HAVE_GNOME_RR_CONFIG_APPLY_WITH_TIME
    gnome_rr_config_apply_with_time(config, screen, (guint32)g_get_real_time(), &error);
#else
    #error No gnome-rr config. application function available!
#endif 

    g_assert_no_error(error);


    g_debug("Applying pixel order.");

    const gchar * const pixel_order = rotation_to_pixel_order(orientation_rot);
    g_debug("pixel order: %s\n", pixel_order);

    if(!set_pixel_order(pixel_order)) {
        g_warning("Failed to set pixel order.");
    }

cleanup:
    g_free(orientation);
    g_free(output_name);

    return result;
}

