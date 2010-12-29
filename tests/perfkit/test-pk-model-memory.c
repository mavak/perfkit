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
	PkModelIter iter;
	PkSample *sample;
	PkSample *sample2;
	PkSample *sample3;
	PkModel *model;
	gint count;

	manifest = pk_manifest_new_from_data(manifest_data, sizeof manifest_data);
	g_assert(manifest);

#define LOAD_SAMPLE(_n)                                  \
    pk_sample_new_from_data(manifest_resolver, manifest, \
                            (_n), sizeof (_n), NULL)

	sample = LOAD_SAMPLE(sample_data);
	g_assert(sample);

	sample2 = LOAD_SAMPLE(sample_data2);
	g_assert(sample2);

	sample3 = LOAD_SAMPLE(sample_data3);
	g_assert(sample3);

	model = g_object_new(PK_TYPE_MODEL_MEMORY, NULL);
	g_assert(model);

	pk_model_insert_manifest(model, manifest);
	pk_model_insert_sample(model, manifest, sample);
	pk_model_insert_sample(model, manifest, sample2);
	pk_model_insert_sample(model, manifest, sample3);

	count = 0;
	if (pk_model_get_iter_first(model, &iter)) {
		do {
			count++;
		} while (pk_model_iter_next(model, &iter));
	}
	g_assert_cmpint(count, ==, 3);

	count = 0;
	if (pk_model_get_iter_for_range(model, &iter, 1293594061.91,
	                                1293594062.9, 0.0)) {
		do {
			count++;
		} while (pk_model_iter_next(model, &iter));
	}
	g_assert_cmpint(count, ==, 2);

	pk_manifest_unref(manifest);
	pk_sample_unref(sample);
	pk_sample_unref(sample2);
	pk_sample_unref(sample3);
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
