/*
 * clibdocker: docker_images.h
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

/**
 * \file docker_images.h
 * \brief Docker Images API
 */

#ifndef SRC_DOCKER_IMAGES_H_
#define SRC_DOCKER_IMAGES_H_

#ifdef __cplusplus  
extern "C" {
#endif

#include <arraylist.h>
#include "docker_connection_util.h"
#include "docker_result.h"
#include "docker_util.h"

#define DEFAULT_DOCKER_FILE_NAME "Dockerfile"

typedef struct docker_image_t {
	char* id;
	char* parent_id;
	arraylist* repo_tags;
	arraylist* repo_digests;
	time_t created;
	unsigned long size;
	unsigned long virtual_size;
	unsigned long shared_size;
	arraylist* labels; //of pair
	unsigned long containers;
} docker_image;

d_err_t make_docker_image(docker_image** image, char* id, char* parent_id,
		time_t created, unsigned long size, unsigned long virtual_size,
		unsigned long shared_size, unsigned long containers);

void free_docker_image(docker_image* image);

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
		arraylist** images, int all, int digests, char* filter_before,
		int filter_dangling, char* filter_label, char* filter_reference,
		char* filter_since);

typedef struct docker_progress_detail_t {
	long current;
	long total;
} docker_progress_detail;

typedef struct docker_image_create_status_t {
	char* status;
	char* id;
	char* progress;
	docker_progress_detail* progress_detail;
} docker_image_create_status;

typedef struct docker_build_status_t {
	char* stream;
	char* aux_id;
} docker_build_status;

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
		docker_result** result, char* from_image, char* tag, char* platform);

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
		void* cbargs, char* from_image, char* tag, char* platform);

//error_t docker_image_create_from_src(docker_context* ctx, docker_result** res, char* from_src, char* repo, char* tag, char* platform);

/**
 * see https://docs.docker.com/engine/api/v1.39/#operation/ImageBuild
 * Build a new image from the files in a folder, with a progress callback
 *
 * \param ctx docker context
 * \param result the docker result object to return
 * \param folder the folder containing the docker image build files (if NULL, default is current directory)
 * \param dockerfile name of the dockerfile. (If NULL, default "Dockerfile" is assumed)
 * \param status_cb callback to call for updates
 * \param cbargs callback args for the upate call
 * \param rest options to the build command
 * \return error code.
 */
d_err_t docker_image_build_cb(docker_context* ctx, docker_result** result,
		char* folder, char* dockerfile,
		void (*status_cb)(docker_build_status*, void* cbargs),
		void* cbargs, ...);

#ifdef __cplusplus 
}
#endif

#endif /* SRC_DOCKER_IMAGES_H_ */