/*
 * sensord
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <common.h>
#include <dbus_util.h>
#include <gio/gio.h>

#define SENSORD_BUS_NAME              "org.tizen.system.sensord"
#define SENSORD_OBJECT_PATH           "/Org/Tizen/System/SensorD"
#define SENSORD_INTERFACE_NAME        SENSORD_BUS_NAME

static GDBusNodeInfo *introspection_data = NULL;
static guint owner_id;

static const gchar introspection_xml[] =
"<node>"
"  <interface name='org.tizen.system.sensord'>"
"	 <method name='check_privilege'>"
"	   <arg type='i' name='response' direction='out'/>"
"	 </method>"
"  </interface>"
"</node>";

static void method_call_handler(GDBusConnection *conn,
				const gchar *sender, const gchar *object_path,
				const gchar *iface_name, const gchar *method_name,
				GVariant *parameters, GDBusMethodInvocation *invocation,
				gpointer user_data)
{
	int ret = DBUS_INIT;

	if (g_strcmp0(method_name, "check_privilege") == 0) {
		_D("check_privilege called");
		ret = DBUS_SUCCESS;
	} else {
		_D("No matched method call");
		ret = DBUS_FAILED;
	}

	g_dbus_method_invocation_return_value(invocation,
					g_variant_new("(i)", ret));

}

static const GDBusInterfaceVTable interface_vtable =
{
	method_call_handler,
	NULL,
	NULL
};

static void on_bus_acquired (GDBusConnection *connection,
		const gchar     *name,
		gpointer         user_data)
{
	guint registration_id;

	if (!connection) {
		_E("connection is null");
		return;
	}

	registration_id = g_dbus_connection_register_object(connection,
		SENSORD_OBJECT_PATH,
		introspection_data->interfaces[0],
		&interface_vtable,
		NULL,  /* user_data */
		NULL,  /* user_data_free_func */
		NULL); /* GError** */

	if (registration_id == 0)
		_E("Failed to g_dbus_connection_register_object");

	_I("Gdbus method call registrated");
}

static void on_name_acquired(GDBusConnection *conn,
				const gchar *name, gpointer user_data)
{
}

static void on_name_lost(GDBusConnection *conn,
				const gchar *name, gpointer user_data)
{
	_E("Dbus name is lost!");
}

void init_dbus(void)
{
	g_type_init();

	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	if (introspection_data == NULL) {
		_E("Failed to init g_dbus_node_info_new_for_xml");
		return;
	}

	owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
							   SENSORD_BUS_NAME,
							   (GBusNameOwnerFlags) (G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT
							   | G_BUS_NAME_OWNER_FLAGS_REPLACE),
							   on_bus_acquired,
							   on_name_acquired,
							   on_name_lost,
							   NULL,
							   NULL);
}

void fini_dbus(void)
{
	if (owner_id != 0)
		g_bus_unown_name(owner_id);

	if (introspection_data)
		g_dbus_node_info_unref(introspection_data);
}
