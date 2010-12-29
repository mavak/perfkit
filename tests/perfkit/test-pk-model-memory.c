#include <perfkit/perfkit.h>

static void
test_PkModelMemory_new (void)
{
	PkModel *model;
	model = g_object_new(PK_TYPE_MODEL_MEMORY, NULL);
	g_assert(model);
	g_object_unref(model);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_setenv("PERFKIT_CONNECTIONS_DIR", PERFKIT_CONNECTIONS_DIR, FALSE);
	g_type_init();
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/PkModelMemory/new",
	                test_PkModelMemory_new);
	return g_test_run();
}
