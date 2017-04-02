/* repo-summary.c:
 *
 * Copyright (C) 2017 Matthias Clasen
 *
 * Licensed under the GNU General Public License Version 3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#define OSTREE_SUMMARY_GVARIANT_STRING "(a(s(taya{sv}))a{sv})"
#define OSTREE_SUMMARY_GVARIANT_FORMAT G_VARIANT_TYPE (OSTREE_SUMMARY_GVARIANT_STRING)

static void
usage (void)
{
        g_print ("Usage: repo-summary REPO [BRANCH]\n");
        exit (1);
}

static void
print_summary (GVariant   *summary,
               const char *branch)
{
        g_autoptr(GVariant) meta = NULL;
        g_autoptr(GVariant) cache = NULL;
        const char *title;
        const char *default_branch;

        meta = g_variant_get_child_value (summary, 1);

        if (branch == NULL) {
                if (g_variant_lookup (meta, "xa.title", "&s", &title))
                        g_print ("Title: %s\n", title);

                if (g_variant_lookup (meta, "xa.default-branch", "&s", &default_branch))
                        g_print ("Default branch: %s\n", default_branch);
        }

        cache = g_variant_lookup_value (meta, "xa.cache", NULL);
        if (cache) {
                g_autoptr(GVariant) refdata = NULL;
                GVariantIter iter;
                const char *ref;
                guint64 installed_size;
                guint64 download_size;
                const char *metadata;

                refdata = g_variant_get_variant (cache);
                if (branch == NULL)
                        g_print ("%zd branches\n", g_variant_n_children (refdata));

                g_variant_iter_init (&iter, refdata);
                while (g_variant_iter_next (&iter, "{&s(tt&s)}", &ref, &installed_size, &download_size, &metadata)) {
                        g_autofree char *installed = g_format_size (GUINT64_FROM_BE (installed_size));
                        g_autofree char *download = g_format_size (GUINT64_FROM_BE (download_size));

                        if (branch == NULL || strcmp (branch, ref) == 0) {
                                g_print ("%s (installed: %s, download: %s)\n", ref, installed, download);
                                if (branch)
                                        g_print ("%s", metadata);
                        }
                }
        }
}

int
main (int argc, char *argv[])
{
        g_autofree char *file = NULL;
        g_autofree char *data = NULL;
        gsize size;
        const char *branch = NULL;
        g_autoptr(GVariant) summary = NULL;
        g_autoptr(GError) error = NULL;

        if (argc < 2 || argc > 3)
                usage ();

        file = g_build_filename (argv[1], "summary", NULL);
        if (argc == 3)
                branch = argv[2];

        if (!g_file_get_contents (file, &data, &size, &error)) {
                g_print ("Error: %s\n", error->message);
                exit (1);
        }

        summary = g_variant_new_from_data (OSTREE_SUMMARY_GVARIANT_FORMAT,
                                           data, size,
                                           FALSE, NULL, NULL);
        g_variant_ref_sink (summary);

        print_summary (summary, branch);

        return 0;
}
