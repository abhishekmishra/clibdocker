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

/**
 * \file docker_system.h
 * \brief Docker System API
 */

#ifndef SRC_DOCKER_SYSTEM_H_
#define SRC_DOCKER_SYSTEM_H_

#ifdef __cplusplus  
extern "C" {
#endif

//TODO: specific API headers should not include docker_all
#include "docker_all.h"

#include <json-c/json_object.h>

/**
* @brief Ping the docker server. Returns E_SUCCESS(0) if the server is reachable.
*
* \param ctx docker context
* \return error code
*/
MODULE_API d_err_t docker_ping(docker_context* ctx);

/**
 * @brief Docker Version json object.
 * 
 * To access members of the docker version object use the
 * getter methods of the form \c docker_version_<member>_get
 */
typedef json_object									docker_version;

/**
 * @brief Free the docker version object.
 */
#define free_docker_version(version)				json_object_put((json_object*) version)

/**
 * @brief Get the docker version string from the docker version object.
 * 
 * @param version docker version
 * @return char* docker version string
 */
#define docker_version_version_get(version)			get_attr_str((json_object*)version, "Version")

/**
 * @brief Get the operating system of the docker server from the docker version object.
 * 
 * @param version docker version
 * @return char* operating system name
 */
#define docker_version_os_get(version)				get_attr_str((json_object*)version, "Os")

/**
 * @brief Get the kernel version string from the docker version object.
 * 
 * @param version docker version
 * @return char* kernel version string
 */
#define docker_version_kernel_version_get(version)	get_attr_str((json_object*)version, "KernelVersion")

/**
 * @brief Get the golang version string from the docker version object.
 * 
 * @param version docker version
 * @return char* golang version string
 */
#define docker_version_go_version_get(version)		get_attr_str((json_object*)version, "GoVersion")

/**
 * @brief Get the git commit id from the docker version object.
 * 
 * @param version docker version
 * @return char* git commit id
 */
#define docker_version_git_commit_get(version)		get_attr_str((json_object*)version, "GitCommit")

/**
 * @brief Get the architecture of the docker server machine from the docker version object.
 * 
 * @param version docker version
 * @return char* architecture of the docker server machine
 */
#define docker_version_arch_get(version)			get_attr_str((json_object*)version, "Arch")

/**
 * @brief Get the api version of the docker server from the docker version object.
 * 
 * @param version docker version
 * @return char* docker server api version
 */
#define docker_version_api_version_get(version)		get_attr_str((json_object*)version, "ApiVersion")

/**
 * @brief Get the minimum api version supported by the docker server from the docker version object.
 * 
 * @param version docker version
 * @return char* minimum api version supported
 */
#define docker_version_min_api_version_get(version) get_attr_str((json_object*)version, "MinAPIVersion")

/**
 * @brief Get the build time of the docker server from the docker version object.
 * 
 * @param version docker version
 * @return char* build time of the docker server
 */
#define docker_version_build_time_get(version)		get_attr_str((json_object*)version, "BuildTime")

/**
 * @brief Get the flag indicating if the docker is running in experimental mode from the docker version object.
 * 
 * @param version docker version
 * @return bool flag indicating experimental mode
 */
#define docker_version_experimental_get(version)	get_attr_boolean((json_object*)version, "Experimental")

/**
 * Gets the docker version information
 *
 * \param ctx docker context
 * \param version docker version object to return
 * \return error code.
 */
MODULE_API d_err_t docker_system_version(docker_context* ctx,
		docker_version** version);

/**
 * @brief Docker Info json object.
 * This object represents the response returned from a docker system info call.
 * 
 * To access the members of docker info use the getters of the form
 * \c docker_info_<member>_get
 */
typedef json_object									docker_info;

/**
 * @brief Free the docker info object
 */
#define free_docker_info(info)						json_object_put((json_object*) info)

/**
 * @brief Get the containers count from the docker info
 * 
 * @param info docker info object
 * @return unsigned long containers count
 */
#define docker_info_containers_get(info)			get_attr_unsigned_long((json_object*)info, "Containers")

/**
 * @brief Get the running containers count from the docker info
 * 
 * @param info docker info object
 * @return unsigned long running containers count
 */
#define docker_info_containers_running_get(info)	get_attr_unsigned_long((json_object*)info, "ContainersRunning")

/**
 * @brief Get the paused containers count from the docker info
 * 
 * @param info docker info object
 * @return unsigned long paused containers count
 */
#define docker_info_containers_paused_get(info)		get_attr_unsigned_long((json_object*)info, "ContainersPaused")

/**
 * @brief Get the stopped containers count from the docker info
 * 
 * @param info docker info object
 * @return unsigned long stopped containers count
 */
#define docker_info_containers_stopped_get(info)	get_attr_unsigned_long((json_object*)info, "ContainersStopped")

/**
 * @brief Get the images count from the docker info
 * 
 * @param info docker info object
 * @return unsigned long images count
 */
#define docker_info_images_get(info)				get_attr_unsigned_long((json_object*)info, "Images")

/**
 * @brief Get the docker server name from the docker info
 * 
 * @param info docker info object
 * @return char* docker server name
 */
#define docker_info_name_get(info)					get_attr_str((json_object*)info, "Name")

/**
 * @brief Get the docker server CPU count from the docker info
 * 
 * @param info docker info object
 * @return int CPU count
 */
#define docker_info_ncpu_get(info)					get_attr_int((json_object*)info, "NCPU")

/**
 * @brief Get the docker server total memory from the docker info
 * 
 * @param info docker info object
 * @return unsigned long total docker server memory
 */
#define docker_info_mem_total_get(info)				get_attr_unsigned_long((json_object*)info, "MemTotal")

/**
 * Gets the docker system information
 *
 * \param ctx docker context
 * \param info object to return
 * \return error code.
 */
MODULE_API d_err_t docker_system_info(docker_context* ctx,
		docker_info** info);

// Docker System Events API

/**
 * @brief Docker Event List json object
 * 
 */
typedef arraylist									docker_event_list;

/**
 * @brief Free the docker event list object
 * 
 * @param event_ls docker event list
 */
#define free_docker_event_list(event_ls)			arraylist_free(event_ls)

/**
 * @brief Length of docker event list
 * 
 * @param event_ls docker event list
 * @return size_t length of the events list
 */
#define docker_event_list_length(event_ls)			arraylist_length(event_ls)

/**
 * @brief Get the ith event from the docker event list
 * 
 * @param event_ls docker event list
 * @param i index
 * @return docker_event* ith event in the list
 */
#define docker_event_list_get_idx(event_ls, i)		arraylist_get(event_ls, i)

/**
 * @brief Docker Event json object
 */
typedef json_object									docker_event;

/**
 * @brief Free the docker event json object
 * 
 * @param event docker event object
 */
#define free_docker_event(event)					json_object_put((json_object*) event)

/**
 * @brief Get the event type string from the docker event object
 * 
 * @param event docker event object
 * @return char* event type
 */
#define docker_event_type_get(event)				get_attr_str((json_object*)event, "Type")

/**
 * @brief Get the action from the docker event object
 * 
 * @param event docker event object
 * @return char* action
 */
#define docker_event_action_get(event)				get_attr_str((json_object*)event, "Action")

/**
 * @brief Get the event time from the docker event object
 * 
 * @param event docker event object
 * @return unsigned long event time
 */
#define docker_event_time_get(event)				get_attr_unsigned_long((json_object*)event, "time")

/**
 * @brief Get the actor id from the docker event object
 * 
 * @param event docker event object
 * @return char* actor id
 */
#define docker_event_actor_id_get(event)			get_attr_str(get_attr_json_object((json_object*)event, "Actor"), "ID")


/**
 * @brief Get the actor attributes from the docker event object
 * 
 * @param event docker event object
 * @return json_object* actor attributes
 */
#define docker_event_attributes_get(event)			get_attr_json_object(get_attr_json_object((json_object*)event, "Actor"), "Attributes")

/**
 * @brief Iterate over the key/val pair of each attribute in the docker event attributes object
 * 
 * @param event docker event object
 */
#define docker_event_attributes_foreach(event)		json_object_object_foreach(docker_event_attributes_get(event), key, val)

/**
 * Get the docker events in a time range.
 *
 * \param ctx the docker context
 * \param events is an array_list containing objects of type docker_event
 * \param start_time
 * \param end_time
 * \return error code
 */
MODULE_API d_err_t docker_system_events(docker_context* ctx,
		arraylist** events, time_t start_time, time_t end_time);

/**
 * Get the docker events in a time range.
 *
 * \param ctx the docker context
 * \param docker_events_cb pointer to callback when an event is received.
 * \param cbargs is a pointer to callback arguments
 * \param events is an array_list containing objects of type docker_event
 * \param start_time
 * \param end_time
 * \return error code
 */
MODULE_API d_err_t docker_system_events_cb(docker_context* ctx,
		void (*docker_events_cb)(docker_event* evt, void* cbargs), void* cbargs,
		arraylist** events, time_t start_time, time_t end_time);

//Docker df

/**
 * @brief Docker disk usage (df) json object
 * 
 * Represents the result of the docker API \c df call.
 * The json object contains the df objects for:
 * 	- Images
 * 	- Volumes (and their usage)
 * 	- Containers
 */
typedef json_object										docker_df;

/**
 * @brief Docker image disk usage (image df) json object
 */
typedef json_object										docker_df_image;

/**
 * @brief Docker container disk usage (container df) json object
 */
typedef json_object										docker_df_container;

/**
 * @brief Docker volume disk usage (volume df) json object
 */
typedef json_object										docker_df_volume;

/**
 * @brief Docker volume usage (volume usage df) json object
 */
typedef json_object										docker_df_volume_usage_data;

/**
 * @brief Get the image id from the docker df image object
 * 
 * @param img docker df image object
 * @return char* image id
 */
#define docker_df_image_id_get(img)						get_attr_str((json_object*)img, "Id")

/**
 * @brief Get the parent image id from the docker df image object
 * 
 * @param img docker df image object
 * @return char* parent image id
 */
#define docker_df_image_parent_id_get(img)				get_attr_str((json_object*)img, "ParentId")

/**
 * @brief Get the image creation datetime from the docker df image object
 * 
 * @param img docker df image object
 * @return unsigned long image creation datetime
 */
#define docker_df_image_created_get(img)				get_attr_unsigned_long((json_object*)img, "Created")

/**
 * @brief Get the image size from the docker df image object
 * 
 * @param img docker df image object
 * @return unsigned long image size
 */
#define docker_df_image_size_get(img)					get_attr_unsigned_long((json_object*)img, "Size")

/**
 * @brief Get the image shared size from the docker df image object
 * 
 * @param img docker df image object
 * @return unsigned long image shared size
 */
#define docker_df_image_shared_size_get(img)			get_attr_unsigned_long((json_object*)img, "SharedSize")

/**
 * @brief Get the image virtual size from the docker df image object
 * 
 * @param img docker df image object
 * @return unsigned long image virtual size
 */
#define docker_df_image_virtual_size_get(img)			get_attr_unsigned_long((json_object*)img, "VirtualSize")

/**
 * @brief Get the image containers count from the docker df image object
 * 
 * @param img docker df image object
 * @return unsigned long image containers count
 */
#define docker_df_image_containers_get(img)				get_attr_unsigned_long((json_object*)img, "Containers")

/**
 * @brief Get the image repo tags list from the docker df image object
 * 
 * @param img docker df image object
 * @return json_object* repo tags list
 */
#define docker_df_image_repo_tags_get(img)				get_attr_json_object((json_object*)img, "RepoTags")
#define docker_df_image_repo_tags_length(img)			json_object_array_length(docker_df_image_repo_tags_get(img))
#define docker_df_image_repo_tags_get_idx(img, i)		(const char*)json_object_array_get_idx(docker_df_image_repo_tags_get(img), i)

#define docker_df_image_repo_digests_get(img)			get_attr_json_object((json_object*)img, "RepoDigests")
#define docker_df_image_repo_digests_length(img)		json_object_array_length(docker_df_image_repo_digests_get(img))
#define docker_df_image_repo_digests_get_idx(img, i)	(const char*)json_object_array_get_idx(docker_df_image_repo_digests_get(img), i)

#define docker_df_container_id_get(ctr)					get_attr_str((json_object*)ctr, "Id")
#define docker_df_container_image_get(ctr)				get_attr_str((json_object*)ctr, "Image")
#define docker_df_container_image_id_get(ctr)			get_attr_str((json_object*)ctr, "ImageID")
#define docker_df_container_command_get(ctr)			get_attr_str((json_object*)ctr, "Command")
#define docker_df_container_created_get(ctr)			get_attr_unsigned_long((json_object*)ctr, "Created")
#define docker_df_container_size_rootfs_get(ctr)		get_attr_unsigned_long((json_object*)ctr, "SizeRootFs")
#define docker_df_container_state_get(ctr)				get_attr_str((json_object*)ctr, "State")
#define docker_df_container_status_get(ctr)				get_attr_str((json_object*)ctr, "Status")

#define docker_df_volume_name_get(vol)					get_attr_str((json_object*)vol, "Name")
#define docker_df_volume_driver_get(vol)				get_attr_str((json_object*)vol, "Driver")
#define docker_df_volume_mountpoint_get(vol)			get_attr_str((json_object*)vol, "Mountpoint")
#define docker_df_volume_scope_get(vol)					get_attr_str((json_object*)vol, "Scope")
#define docker_df_volume_usage_data_get(vol)			(docker_df_volume_usage_data*)get_attr_json_object((json_object*)vol, "UsageData")

#define docker_df_volume_usage_data_size_get(ud)		get_attr_unsigned_long((json_object*)ud, "Size")
#define docker_df_volume_usage_data_ref_count_get(ud)	get_attr_unsigned_long((json_object*)ud, "RefCount")

#define docker_df_container_names_get(ctr)				get_attr_json_object((json_object*)ctr, "Names")
#define docker_df_container_names_length(ctr)			json_object_array_length(docker_df_container_names_get(ctr))
#define docker_df_container_names_get_idx(ctr, i)		(const char*)json_object_array_get_idx(docker_df_container_names_get(ctr), i)

#define docker_df_container_mounts_get(ctr)				get_attr_json_object((json_object*)ctr, "Mounts")
#define docker_df_container_mounts_length(ctr)			json_object_array_length(docker_df_container_mounts_get(ctr))
#define docker_df_container_mounts_get_idx(ctr, i)		(const char*)json_object_array_get_idx(docker_df_container_mounts_get(ctr), i)

#define free_docker_df(df)								json_object_put((json_object*) df)
#define docker_df_layers_size_get(df)					get_attr_unsigned_long((json_object*)df, "LayersSize")

#define docker_df_images_get(df)						get_attr_json_object((json_object*)df, "Images")
#define docker_df_images_length(df)						json_object_array_length(docker_df_images_get(df))
#define docker_df_images_get_idx(df, i)					(docker_df_image*)json_object_array_get_idx(docker_df_images_get(df), i)

#define docker_df_containers_get(df)					get_attr_json_object((json_object*)df, "Containers")
#define docker_df_containers_length(df)					json_object_array_length(docker_df_containers_get(df))
#define docker_df_containers_get_idx(df, i)				(docker_df_container*)json_object_array_get_idx(docker_df_containers_get(df), i)

#define docker_df_volumes_get(df)						get_attr_json_object((json_object*)df, "Volumes")
#define docker_df_volumes_length(df)					json_object_array_length(docker_df_volumes_get(df))
#define docker_df_volumes_get_idx(df, i)				(docker_df_volume*)json_object_array_get_idx(docker_df_volumes_get(df), i)

/**
 * Gets the docker usage data
 *
 * \param ctx docker context
 * \param docker_df object to return
 * \return error code.
 */
MODULE_API d_err_t docker_system_df(docker_context* ctx, docker_df** df);

#ifdef __cplusplus 
}
#endif

#endif /* SRC_DOCKER_SYSTEM_H_ */
