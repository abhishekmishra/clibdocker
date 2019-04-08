/*
 * clibdocker: docker_images.c
 * Created on: 31-Dec-2018
 *
 * clibdocker
 * Copyright (C) 2018 Abhishek Mishra <abhishekmishra3@gmail.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <docker_log.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <archive.h>
#include <archive_entry.h>
#include "string.h"
#include "docker_images.h"
#include "tinydir.h"

d_err_t make_docker_image(docker_image** image, char* id, char* parent_id,
		time_t created, unsigned long size, unsigned long virtual_size,
		unsigned long shared_size, unsigned long containers)
{
	(*image) = (docker_image*) malloc(sizeof(docker_image));
	if ((*image) == NULL)
	{
		return E_ALLOC_FAILED;
	}

	(*image)->id = str_clone(id);
	(*image)->parent_id = str_clone(parent_id);
	(*image)->created = created;
	(*image)->size = size;
	(*image)->virtual_size = virtual_size;
	(*image)->shared_size = shared_size;
	(*image)->containers = containers;

	(*image)->repo_tags = array_list_new(&free);
	(*image)->repo_digests = array_list_new(&free);
	(*image)->labels = array_list_new((void (*)(void *)) &free_pair);

	return E_SUCCESS;
}

void free_docker_image(docker_image* image)
{
	if (image)
	{
		free(image->id);
		free(image->parent_id);
		array_list_free(image->repo_tags);
		array_list_free(image->repo_digests);
		array_list_free(image->labels);
	}
}

/**
 * List images matching the filters.
 *
 * \param ctx docker context
 * \param result the docker result object to return
 * \param images array list of images to be returned
 * \param all (0 indicates false, true otherwise)
 * \param digests add repo digests in return object (0 is false, true otherwise)
 * \param filter_before <image-name>[:<tag>], <image id> or <image@digest>
 * \param filter_dangling 0 is false, true otherwise.
 * \param filter_label label=key or label="key=value" of an image label
 * \param filter_reference <image-name>[:<tag>]
 * \param filter_since <image-name>[:<tag>], <image id> or <image@digest>
 * \return error code
 */
d_err_t docker_images_list(docker_context* ctx, docker_result** result,
		struct array_list** images, int all, int digests, char* filter_before,
		int filter_dangling, char* filter_label, char* filter_reference,
		char* filter_since)
{
	char* url = create_service_url_id_method(IMAGE, NULL, "json");

	struct array_list* params = array_list_new(
			(void (*)(void *)) &free_url_param);
	url_param* p;
	json_object* filters = make_filters();
	if (filter_before != NULL)
	{
		add_filter_str(filters, "before", str_clone(filter_before));
	}
	if (filter_dangling != 0)
	{
		add_filter_str(filters, "dangling", str_clone("true"));
	}
	if (filter_label != NULL)
	{
		add_filter_str(filters, "label", str_clone(filter_label));
	}
	if (filter_reference != NULL)
	{
		add_filter_str(filters, "reference", str_clone(filter_reference));
	}
	if (filter_since != NULL)
	{
		add_filter_str(filters, "since", str_clone(filter_since));
	}
	make_url_param(&p, "filters", (char *) filters_to_str(filters));
	array_list_add(params, p);

	if (all != 0)
	{
		make_url_param(&p, "all", "true");
		array_list_add(params, p);
	}

	if (digests != 0)
	{
		make_url_param(&p, "digests", "true");
		array_list_add(params, p);
	}

	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_get(ctx, result, url, params, &chunk, &response_obj);

	(*images) = array_list_new((void (*)(void *)) &free_docker_image);
	int num_imgs = json_object_array_length(response_obj);

	for (int i = 0; i < num_imgs; i++)
	{
		json_object* current_obj = json_object_array_get_idx(response_obj, i);
		docker_image* img;

		make_docker_image(&img, get_attr_str(current_obj, "Id"),
				get_attr_str(current_obj, "ParentId"),
				get_attr_unsigned_long(current_obj, "Created"),
				get_attr_unsigned_long(current_obj, "Size"),
				get_attr_unsigned_long(current_obj, "VirtualSize"),
				get_attr_unsigned_long(current_obj, "SharedSize"),
				get_attr_unsigned_long(current_obj, "Containers"));

		json_object* labels_obj;
		json_object_object_get_ex(current_obj, "Labels", &labels_obj);
		if (labels_obj != NULL)
		{
			json_object_object_foreach(labels_obj, key, val)
			{
				pair* p;
				make_pair(&p, key, (char*) json_object_get_string(val));
				array_list_add(img->labels, p);
			}
		}
		json_object* repo_tags_obj;
		json_object_object_get_ex(current_obj, "RepoTags", &repo_tags_obj);
		if (repo_tags_obj != NULL)
		{
			int num_repo_tags = json_object_array_length(repo_tags_obj);
			for (int j = 0; j < num_repo_tags; j++)
			{
				array_list_add(img->repo_tags,
						(char*) json_object_get_string(
								json_object_array_get_idx(repo_tags_obj, j)));
			}
		}
		json_object* repo_digests_obj;
		json_object_object_get_ex(current_obj, "RepoDigests",
				&repo_digests_obj);
		if (repo_digests_obj != NULL)
		{
			int num_repo_digests = json_object_array_length(repo_digests_obj);
			for (int j = 0; j < num_repo_digests; j++)
			{
				array_list_add(img->repo_digests,
						(char*) json_object_get_string(
								json_object_array_get_idx(repo_digests_obj,
										j)));
			}
		}
		array_list_add((*images), img);
	}

	return E_SUCCESS;
}

void parse_status_cb(char* msg, void* cb, void* cbargs)
{
	void (*status_cb)(docker_image_create_status*,
			void*) = (void (*)(docker_image_create_status*, void*))cb;
	if (msg)
	{
		if(status_cb)
		{
			json_object* response_obj = json_tokener_parse(msg);
			char* id = get_attr_str(response_obj, "id");
			if(id == NULL)
			{
				char* message = get_attr_str(response_obj, "message");
				status_cb(NULL, cbargs);
			}
			else
			{
				char* status_msg = get_attr_str(response_obj, "status");
				char* progress = get_attr_str(response_obj, "progress");

				docker_image_create_status* status = (docker_image_create_status*)calloc(1, sizeof(docker_image_create_status));
				if(status != NULL)
				{
					status->status = status_msg;
					status->id = id;
					status->progress = progress;
				}

				json_object* progress_detail_obj;
				if (json_object_object_get_ex(response_obj, "progressDetail", &progress_detail_obj) == 1)
				{
					long current = get_attr_long(progress_detail_obj, "current");
					long total = get_attr_long(progress_detail_obj, "total");
					docker_progress_detail* progress_detail = (docker_progress_detail*)calloc(1, sizeof(docker_progress_detail));
					if(progress_detail != NULL)
					{
						progress_detail->current = current;
						progress_detail->total = total;
						status->progress_detail = progress_detail;
					}
				}
				status_cb(status, cbargs);
			}
		}
		else
		{
			docker_log_debug("Message = Empty");
		}
	}
}

//Docker Image Create commands
/**
 * see https://docs.docker.com/engine/api/v1.39/#operation/ImageCreate
 * Create a new image by pulling image:tag for platform
 *
 * \param ctx docker context
 * \param result the docker result object to return
 * \param from_image image name
 * \param tag which tag to pull, for e.g. "latest"
 * \param platform which platform to pull the image for (format os[/arch[/variant]]),
 * 			default is ""
 * \return error code.
 */
d_err_t docker_image_create_from_image(docker_context* ctx,
		docker_result** result, char* from_image, char* tag, char* platform)
{
	return docker_image_create_from_image_cb(ctx, result, NULL, NULL,
			from_image, tag, platform);
}

/**
 * see https://docs.docker.com/engine/api/v1.39/#operation/ImageCreate
 * Create a new image by pulling image:tag for platform, with a progress callback
 *
 * \param ctx docker context
 * \param result the docker result object to return
 * \param status_cb callback to call for updates
 * \param cbargs callback args for the upate call
 * \param from_image image name
 * \param tag which tag to pull, for e.g. "latest"
 * \param platform which platform to pull the image for (format os[/arch[/variant]]),
 * 			default is ""
 * \return error code.
 */
d_err_t docker_image_create_from_image_cb(docker_context* ctx,
		docker_result** result,
		void (*status_cb)(docker_image_create_status*, void* cbargs),
		void* cbargs, char* from_image, char* tag, char* platform)
{
	if (from_image == NULL || strlen(from_image) == 0)
	{
		return E_INVALID_INPUT;
	}

	char* url = create_service_url_id_method(IMAGE, NULL, "create");

	struct array_list* params = array_list_new(
			(void (*)(void *)) &free_url_param);
	url_param* p;

	make_url_param(&p, "fromImage", from_image);
	array_list_add(params, p);
	if (tag != NULL)
	{
		make_url_param(&p, "tag", tag);
		array_list_add(params, p);
	}
	if (platform != NULL)
	{
		make_url_param(&p, " platform", platform);
		array_list_add(params, p);
	}

	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_post_cb(ctx, result, url, params, "", &chunk, &response_obj,
			&parse_status_cb, status_cb, cbargs);

	if ((*result)->http_error_code > 200)
	{
		return E_UNKNOWN_ERROR;
	}

	return E_SUCCESS;
}

array_list* list_dir(char* folder_path)
{
	array_list* paths = array_list_new(&free);
//	printf("To list %s\n", folder_path);

	tinydir_dir dir;
	tinydir_open(&dir, folder_path);

	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		if (strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0)
		{
//			printf("%s", file.name);
			char* sub_dir_path = (char*) calloc(
					strlen(folder_path) + strlen(file.name) + 2, sizeof(char));
			strcpy(sub_dir_path, folder_path);
			strcat(sub_dir_path, "/");
			strcat(sub_dir_path, file.name);
			if (file.is_dir)
			{
//				printf("/\n");
				array_list* sub_dir_ls = list_dir(sub_dir_path);
				int ls_count = array_list_length(sub_dir_ls);
				for (int i = 0; i < ls_count; i++)
				{
					array_list_add(paths,
							str_clone(array_list_get_idx(sub_dir_ls, i)));
				}
				free(sub_dir_ls);
			} else {
//				printf("\n");
			}
			array_list_add(paths, sub_dir_path);
		}
		tinydir_next(&dir);
	}

	tinydir_close(&dir);

	return paths;
}

/**
 * see https://docs.docker.com/engine/api/v1.39/#operation/ImageBuild
 * Build a new image from the files in a folder, with a progress callback
 *
 * \param ctx docker context
 * \param result the docker result object to return
 * \param folder the folder containing the docker image build files
 * \param dockerfile name of the dockerfile. (If NULL, default "Dockerfile" is assumed)
 * \param status_cb callback to call for updates
 * \param cbargs callback args for the upate call
 * \param rest options to the build command
 * \return error code.
 */
d_err_t docker_image_build_cb(docker_context* ctx, docker_result** result,
		char* folder, char* dockerfile,
		void (*status_cb)(docker_image_create_status*, void* cbargs),
		void* cbargs, ...)
{
	char* url = create_service_url_id_method(SYSTEM, NULL, "build");
	char* folder_path = ".";
	char* docker_file_name = DEFAULT_DOCKER_FILE_NAME;

	struct array_list* params = array_list_new(
			(void (*)(void *)) &free_url_param);
	url_param* p;

	//Get folder and dockerfile name
	if (folder != NULL)
	{
		folder_path = folder;
	}

	if (dockerfile != NULL)
	{
		docker_file_name = dockerfile;
	}

	//Add dockerfile as param to the request, if not NULL

	//Create tarball from folder into a buffer
	struct archive *a;
	struct archive_entry *entry;
	//TODO check platform compatibility of stat
	struct stat st;
	char buff[8192];
	int len;
	int fd;

	a = archive_write_new();
//	archive_write_add_filter_gzip(a);
	archive_write_set_format_pax_restricted(a); // Note 1
	archive_write_open_filename(a, "/home/abhishek/tmp/test.tar");
	array_list* sub_dir_ls = list_dir(folder_path);
	int ls_count = array_list_length(sub_dir_ls);
	for (int i = 0; i < ls_count; i++)
	{
		char* filename = array_list_get_idx(sub_dir_ls, i);
		printf("%s\n", filename);
		stat(filename, &st);
		entry = archive_entry_new(); // Note 2
		archive_entry_set_pathname(entry, filename);
		archive_entry_set_size(entry, st.st_size); // Note 3
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0644);
		archive_write_header(a, entry);
		fd = open(filename, O_RDONLY);
		len = read(fd, buff, sizeof(buff));
		while (len > 0)
		{
			archive_write_data(a, buff, len);
			len = read(fd, buff, sizeof(buff));
		}
		close(fd);
		archive_entry_free(entry);
		filename++;
	}
	archive_write_close(a); // Note 4
	archive_write_free(a); // Note 5
	free(sub_dir_ls);

	//Post tarball buffer to the API
	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_post_cb(ctx, result, url, params, "", &chunk, &response_obj,
			&parse_status_cb, status_cb, cbargs);

	if ((*result)->http_error_code > 200)
	{
		return E_UNKNOWN_ERROR;
	}

	return E_SUCCESS;
}
