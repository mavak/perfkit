#include <perfkit/perfkit.h>

#include "manifest.h"
#include "sample.h"

static void
test_PkModelMemory_insert_manifest (void)
{
	PkManifest *manifest;
	PkModel *model;

	manifest = pk_manifest_new_from_data(manifest_data, sizeof(manifest_data));
	g_assert(manifest);

	model = g_object_new(PK_TYPE_MODEL_MEMORY, NULL);
	g_assert(model);

	pk_model_insert_manifest(model, manifest);

	pk_manifest_unref(manifest);
	g_object_unref(model);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_setenv("PERFKIT_CONNECTIONS_DIR", PERFKIT_CONNECTIONS_DIR, FALSE);
	g_type_init();
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/PkModelMemory/insert_manifest",
	                test_PkModelMemory_insert_manifest);
	return g_test_run();
}
