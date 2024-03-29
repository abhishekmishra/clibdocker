/*
 *
 * Copyright (c) 2018-2022 Abhishek Mishra
 *
 * This file is part of clibdocker.
 *
 * clibdocker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) 
 * any later version.
 *
 * clibdocker is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public 
 * License along with clibdocker. 
 * If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "lua_docker.h"
#include "docker_log.h"

void *check_JsonObject(lua_State *L, int i)
{
	return luaL_checkudata(L, i, JsonObject_metatable);
}

void *check_DockerClient(lua_State *L, int i)
{
	return luaL_checkudata(L, i, DockerClient_metatable);
}

int DockerClient_connect_url(lua_State *L)
{
	const char *url = lua_tostring(L, 1);
	lua_pop(L, 1);

	// Create a DockerClient instance and set its metatable.
	DockerClient *dc = (DockerClient *)lua_newuserdata(L, sizeof(DockerClient));
	// stack = [dc]
	luaL_getmetatable(L, DockerClient_metatable);
	// stack = [dc, mt]
	lua_setmetatable(L, 1);
	// stack = [dc]

	d_err_t err = make_docker_context_url(&(dc->ctx), url);
	if (err != E_SUCCESS)
	{
		luaL_error(L, "Error creating docker client");
	}
	return 1;
}

int DockerClient_new(lua_State *L)
{
	// Create a DockerClient instance and set its metatable.
	DockerClient *dc = (DockerClient *)lua_newuserdata(L, sizeof(DockerClient));
	// stack = [dc]
	luaL_getmetatable(L, DockerClient_metatable);
	// stack = [dc, mt]
	lua_setmetatable(L, 1);
	// stack = [dc]

	d_err_t err = make_docker_context_default_local(&(dc->ctx));
	if (err != E_SUCCESS)
	{
		luaL_error(L, "Error creating docker client");
	}
	return 1;
}

int DockerClient_from_context(lua_State* L, docker_context* ctx) {
	// Create a DockerClient instance and set its metatable.
	DockerClient *dc = (DockerClient *)lua_newuserdata(L, sizeof(DockerClient));
	// stack = [dc]
	luaL_getmetatable(L, DockerClient_metatable);
	// stack = [dc, mt]
	lua_setmetatable(L, 1);
	// stack = [dc]

	dc->ctx = ctx;
	
	return 1;
}

int JsonObject__gc(lua_State *L)
{
	printf("In JsonObject__gc\n");
	JsonObject *jo = check_JsonObject(L, 1);
	json_object_put(jo->obj);
	return 0;
}

int JsonObject_json_create(lua_State *L)
{
	const char *json_str = lua_tostring(L, 1);
	lua_pop(L, 1);

	// Create a JsonObject instance and set its metatable.
	JsonObject *jo = (JsonObject *)lua_newuserdata(L, sizeof(JsonObject));
	// stack = [jo]
	luaL_getmetatable(L, JsonObject_metatable);
	// stack = [jo, mt]
	lua_setmetatable(L, 1);
	// stack = [jo]

	jo->obj = json_tokener_parse(json_str);

	if (jo->obj == NULL)
	{
		luaL_error(L, "Error parsing json object.");
	}
	return 1;
}

int JsonObject_json_string(lua_State *L)
{
	JsonObject *jo = check_JsonObject(L, 1);
	const char *jo_str = get_json_string(jo->obj);
	lua_pushstring(L, jo_str);
	return 1;
}

int DockerClient__gc(lua_State *L)
{
	DockerClient *dc = check_DockerClient(L, 1);
	free_docker_context(&(dc->ctx));
	return 0;
}

int DockerClient_loglevel(lua_State *L)
{
	// Expected: stack = [self, int loglevel]
	DockerClient *dc = (DockerClient *)luaL_checkudata(L, 1, DockerClient_metatable);
	int loglevel = (int)luaL_checkinteger(L, 2);

	docker_log_set_level(loglevel);
	return 0;
}

//this method does not pass the container list
//filters, for using filters use 
//DockerClient_container_list_filter
int DockerClient_container_list(lua_State *L)
{
	docker_log_debug("%d parameters received", lua_gettop(L));
	// Expected: stack = [self, boolean all, boolean limit, boolean size]
	DockerClient *dc = (DockerClient *)luaL_checkudata(L, 1, DockerClient_metatable);
	int all = lua_toboolean(L, 2);
	int limit = (int)luaL_checkinteger(L, 3);
	int size = lua_toboolean(L, 4);

	docker_ctr_list *ctrls;
	d_err_t err = docker_container_list(dc->ctx, &ctrls, all, limit, size, NULL);
	if (err != E_SUCCESS)
	{
		luaL_error(L, "Error listing containers");
	}
	const char *ctrls_json = get_json_string(ctrls);
	lua_pushstring(L, ctrls_json);
	return 1;
}

int DockerClient_container_list_filter(lua_State *L)
{
	docker_log_debug("%d parameters received", lua_gettop(L));
	// Expected: stack = [self, boolean all, boolean limit, boolean size, table filters]
	DockerClient *dc = (DockerClient *)luaL_checkudata(L, 1, DockerClient_metatable);
	int all = lua_toboolean(L, 2);
	int limit = (int)luaL_checkinteger(L, 3);
	int size = lua_toboolean(L, 4);
	const char* filter_json_str = lua_tostring(L, 5);

	docker_ctr_list *ctrls;
	d_err_t err = docker_container_list_filter_str(dc->ctx, &ctrls, all, limit, size, filter_json_str);
	if (err != E_SUCCESS)
	{
		luaL_error(L, "Error listing containers");
	}
	const char *ctrls_json = get_json_string(ctrls);
	lua_pushstring(L, ctrls_json);
	return 1;
}

int DockerClient_create_container(lua_State *L)
{
	// Expected: stack = [self, json_object]
	DockerClient *dc = check_DockerClient(L, 1);
	JsonObject *jo = check_JsonObject(L, 2);

	char *id;
	d_err_t err = docker_create_container(dc->ctx, &id, jo->obj);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Error listing containers");
	}
	
	lua_pushstring(L, id);
	return 1;
}

int DockerClient_inspect_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Getting container details for %s\n", id);

	docker_ctr* ctr = docker_inspect_container(dc->ctx, (char *)id, 1);

	if (ctr == NULL)
	{
		luaL_error(L, "Unable to inspect container id %s", id);
	}
	const char *ctr_json = get_json_string(ctr);
	lua_pushstring(L, ctr_json);
	return 1;
}

int DockerClient_process_list_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Getting process details for %s\n", id);

	docker_ctr_ps *ps;
	d_err_t err = docker_process_list_container(dc->ctx, &ps, (char *)id, NULL);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to get process details container id %s", id);
	}
	const char *ps_json = get_json_string(ps);
	lua_pushstring(L, ps_json);
	return 1;
}

int DockerClient_container_logs(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Getting logs for %s\n", id);

	char* log;
	size_t log_length;
	d_err_t err = docker_container_logs(dc->ctx, &log, &log_length, (char*)id, 0,
		1, 1, -1, -1, 1, 0);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to get logs for container id %s", id);
	}
	lua_pushlstring(L, log, log_length);
	return 1;
}

int DockerClient_start_container(lua_State *L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Starting container id %s\n", id);

	d_err_t err = docker_start_container(dc->ctx, (char *)id, NULL);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to start container id %s", id);
	}
	return 0;
}

int DockerClient_stop_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Stopping container id %s\n", id);

	d_err_t err = docker_stop_container(dc->ctx, (char *)id, 0);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to stop container id %s", id);
	}
	return 0;
}

int DockerClient_restart_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Restarting container id %s\n", id);

	d_err_t err = docker_restart_container(dc->ctx, (char *)id, 0);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to restart container id %s", id);
	}
	return 0;
}

int DockerClient_kill_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Kiling container id %s\n", id);

	d_err_t err = docker_kill_container(dc->ctx, (char *)id, NULL);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to kill container id %s", id);
	}
	return 0;
}

int DockerClient_rename_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);
	const char *name = lua_tostring(L, 3);

	docker_log_info("Renaming container id %s\n", id);

	d_err_t err = docker_rename_container(dc->ctx, (char *)id, (char*)name);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to rename container id %s", id);
	}
	return 0;
}

int DockerClient_pause_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Pausing container id %s\n", id);

	d_err_t err = docker_pause_container(dc->ctx, (char *)id);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to pause container id %s", id);
	}
	return 0;
}

int DockerClient_unpause_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("UnPausing container id %s\n", id);

	d_err_t err = docker_unpause_container(dc->ctx, (char *)id);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to unpause container id %s", id);
	}
	return 0;
}

int DockerClient_wait_container(lua_State* L)
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Wait for container id %s\n", id);

	d_err_t err = docker_wait_container(dc->ctx, (char *)id, NULL);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to wait container id %s", id);
	}
	return 0;
}

int DockerClient_remove_container(lua_State* L) 
{
	// Expected: stack = [self, id]
	DockerClient *dc = check_DockerClient(L, 1);
	const char *id = lua_tostring(L, 2);

	docker_log_info("Removing container id %s\n", id);

	d_err_t err = docker_remove_container(dc->ctx, (char *)id, 0, 0, 0);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Unable to remove container id %s", id);
	}
	return 0;
}

int DockerClient_image_ls(lua_State* L)
{
	// Expected: stack = [self, boolean all, boolean digests]
	DockerClient* dc = check_DockerClient(L, 1);
	int all = lua_toboolean(L, 2);
	int digests = lua_toboolean(L, 3);

	docker_log_info("Listing images with params all=%d digests=%d\n", all, digests);

	docker_image_list* images;
	d_err_t err = docker_images_list(dc->ctx, &images, all, digests, 
		NULL, 0, NULL, NULL, NULL);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Error listing images");
	}

	const char* images_json = get_json_string(images);
	lua_pushstring(L, images_json);

	return 1;
}

int DockerClient_image_pull(lua_State* L)
{
	// Expected: stack = [self, string image, string tag, string platform]
	DockerClient* dc = check_DockerClient(L, 1);
	const char *image = luaL_checkstring(L, 2);
	const char *tag = luaL_optstring(L, 3, "latest");
	const char *platform = luaL_optstring(L, 4, NULL);

	docker_log_info("Pulling image %s, tag=%s, platform=%s\n", image, tag, platform);

	d_err_t err = docker_image_create_from_image(dc->ctx, image, tag, platform);

	if (err != E_SUCCESS)
	{
		luaL_error(L, "Error pulling image %s, tag=%s, platform=%s\n", image, tag, platform);
	}

	return 0;
}

int luaopen_luaclibdocker(lua_State *L)
{
	static const luaL_Reg JsonObject_lib[] = {
		{"to_string", &JsonObject_json_string},
		{NULL, NULL}};

	static const luaL_Reg DockerClient_lib[] = {
		{"set_loglevel", &DockerClient_loglevel},
		{"container_ls", &DockerClient_container_list},
		{"container_ls_filter", &DockerClient_container_list_filter},
		{"container_create", &DockerClient_create_container},
		{"container_inspect", &DockerClient_inspect_container},
		{"container_top", &DockerClient_process_list_container},
		{"container_logs_raw", &DockerClient_container_logs},
		{"container_start", &DockerClient_start_container},
		{"container_stop", &DockerClient_stop_container},
		{"container_restart", &DockerClient_restart_container},
		{"container_kill", &DockerClient_kill_container},
		{"container_rename", &DockerClient_rename_container},
		{"container_pause", &DockerClient_pause_container},
		{"container_unpause", &DockerClient_unpause_container},
		{"container_wait", &DockerClient_wait_container},
		{"container_remove", &DockerClient_remove_container},
		{"image_ls", &DockerClient_image_ls},
		{"image_pull", &DockerClient_image_pull},
		{NULL, NULL}};

	static const luaL_Reg clibdocker_lib[] = {
		{"connect", &DockerClient_new},
		{"connect_url", &DockerClient_connect_url},
		{"json_create", &JsonObject_json_create},
		{NULL, NULL}};

	luaL_newlib(L, clibdocker_lib);

	// Stack: clibdocker
	luaL_newmetatable(L, JsonObject_metatable); // Stack: clibdocker meta
	luaL_newlib(L, JsonObject_lib);
	lua_setfield(L, -2, "__index"); // Stack: clibdocker meta

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, JsonObject__gc); // Stack: clibdocker meta "__gc" fptr
	lua_settable(L, -3);				  // Stack: clibdocker meta
	lua_pop(L, 1);						  // Stack: clibdocker

	// Stack: clibdocker
	luaL_newmetatable(L, DockerClient_metatable); // Stack: clibdocker meta
	luaL_newlib(L, DockerClient_lib);
	lua_setfield(L, -2, "__index"); // Stack: clibdocker meta

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, DockerClient__gc); // Stack: clibdocker meta "__gc" fptr
	lua_settable(L, -3);					// Stack: clibdocker meta
	lua_pop(L, 1);							// Stack: clibdocker

	return 1;
}