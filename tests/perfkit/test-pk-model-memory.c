#include <perfkit/perfkit.h>

#include "manifest.h"
#include "sample.h"

static gboolean
manifest_resolver (gint         source_id,
                   PkManifest **manifest,
                   gpointer     real_manifest)
{
	*manifest = real_manifest;
	return TRUE;
}

static void
test_PkModelMemory_insert_tests (void)
{
	PkManifest *manifest;
	PkSample *sample;
	PkModel *model;

	manifest = pk_manifest_new_from_data(manifest_data, sizeof manifest_data);
	g_assert(manifest);

	sample = pk_sample_new_from_data(manifest_resolver, manifest,
	                                 sample_data, sizeof sample_data,
	                                 NULL);
	g_assert(sample);

	model = g_object_new(PK_TYPE_MODEL_MEMORY, NULL);
	g_assert(model);

	pk_model_insert_manifest(model, manifest);
	pk_model_insert_sample(model, manifest, sample);

	pk_manifest_unref(manifest);
	pk_sample_unref(sample);
	g_object_unref(model);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_setenv("PERFKIT_CONNECTIONS_DIR", PERFKIT_CONNECTIONS_DIR, FALSE);
	g_type_init();
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/PkModelMemory/insert_tests",
	                test_PkModelMemory_insert_tests);
	return g_test_run();
}
