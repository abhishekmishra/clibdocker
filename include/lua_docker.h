#ifndef SRC_LUA_DOCKER_H_
#define SRC_LUA_DOCKER_H_

#ifdef LUA_ENABLED

#ifdef __cplusplus  
extern "C" {
#endif

#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"

#include "docker_all.h"

#define DockerClient_metatable "DOCKER_CLIENT"

typedef struct {
	docker_context* ctx;
} DockerClient;

LUALIB_API int DockerClient_connect_url(lua_State* L);

// DockerClient:new()
LUALIB_API int DockerClient_new(lua_State* L);

// container APIs
LUALIB_API int DockerClient_container_list(lua_State* L);
LUALIB_API int DockerClient_create_container(lua_State* L);
LUALIB_API int DockerClient_inspect_container(lua_State* L);
LUALIB_API int DockerClient_process_list_container(lua_State* L);
LUALIB_API int DockerClient_container_logs(lua_State* L);
LUALIB_API int DockerClient_container_changes(lua_State* L);
LUALIB_API int DockerClient_container_stats(lua_State* L);
LUALIB_API int DockerClient_start_container(lua_State* L);
LUALIB_API int DockerClient_stop_container(lua_State* L);
LUALIB_API int DockerClient_restart_container(lua_State* L);
LUALIB_API int DockerClient_kill_container(lua_State* L);
LUALIB_API int DockerClient_rename_container(lua_State* L);
LUALIB_API int DockerClient_pause_container(lua_State* L);
LUALIB_API int DockerClient_unpause_container(lua_State* L);
LUALIB_API int DockerClient_wait_container(lua_State* L);
LUALIB_API int DockerClient_remove_container(lua_State* L);


LUALIB_API int luaopen_luaclibdocker(lua_State* L);

#ifdef __cplusplus 
}
#endif

#endif //LUA_ENABLED

#endif //SRC_LUA_DOCKER_H_
