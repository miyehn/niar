void Pathtracer::set_mainbuffer_rgb(uint32_t i, vec3 rgb) {
	uint32_t pixel_size = NUM_CHANNELS * SIZE_PER_CHANNEL;
	image_buffer[pixel_size * i] = char(rgb.r * 255.0f);
	image_buffer[pixel_size * i + 1] = char(rgb.g * 255.0f);
	image_buffer[pixel_size * i + 2] = char(rgb.b * 255.0f);
	image_buffer[pixel_size * i + 3] = 255;
}

void Pathtracer::set_subbuffer_rgb(uint32_t buf_i, uint32_t i, vec3 rgb) {
	uint32_t pixel_size = NUM_CHANNELS * SIZE_PER_CHANNEL;
	unsigned char* buf = subimage_buffers[buf_i];
	buf[pixel_size * i] = char(rgb.r * 255.0f);
	buf[pixel_size * i + 1] = char(rgb.g * 255.0f);
	buf[pixel_size * i + 2] = char(rgb.b * 255.0f);
	buf[pixel_size * i + 3] = 255;
}

vec3 gamma_correct(vec3 in) {
	const vec3 gamma(0.455f);
	return pow(in, gamma);
}

void Pathtracer::raytrace_tile(uint32_t tid, uint32_t tile_index) {
	uint32_t X = tile_index % tiles_X;
	uint32_t Y = tile_index / tiles_X;

	uint32_t tile_size = cached_config.TileSize;

	uint32_t tile_w = std::min(tile_size, width - X * tile_size);
	uint32_t tile_h = std::min(tile_size, height - Y * tile_size);

	uint32_t x_offset = X * tile_size;
	uint32_t y_offset = Y * tile_size;

	for (uint32_t y = 0; y < tile_h; y++) {
		for (uint32_t x = 0; x < tile_w; x++) {

			uint32_t px_index_main = width * (y_offset + y) + (x_offset + x);
			vec3 color = raytrace_pixel(px_index_main);

			// do gamma correction BEFORE converting to R8G8B8A8 to avoid banding
			color = gamma_correct(color);

			set_mainbuffer_rgb(px_index_main, color);

			uint32_t px_index_sub = y * tile_w + x;
			set_subbuffer_rgb(tid, px_index_sub, color);

		}
	}

}

#if GRAPHICS_DISPLAY

void Pathtracer::upload_rows(uint32_t begin, uint32_t rows)
{
	uint32_t subimage_offset = width * begin * NUM_CHANNELS * SIZE_PER_CHANNEL;
	uint8_t* data = image_buffer + subimage_offset;
	vk::uploadPixelsToImage(
		data,
		0, begin,
		width, rows,
		NUM_CHANNELS *SIZE_PER_CHANNEL,
		window_surface->resource
		);

	int percentage = int(float(begin + rows) / float(height) * 100.0f);
	TRACE("refresh! updated %d rows, %d%% done.", rows, percentage);
}

void Pathtracer::upload_tile(uint32_t subbuf_index, uint32_t begin_x, uint32_t begin_y, uint32_t w, uint32_t h)
{
	unsigned char* buffer = subimage_buffers[subbuf_index];
	vk::uploadPixelsToImage(
		buffer,
		begin_x, begin_y,
		w, h,
		NUM_CHANNELS *SIZE_PER_CHANNEL,
		window_surface->resource
	);
}

void Pathtracer::upload_tile(uint32_t subbuf_index, uint32_t tile_index) {
	uint32_t X = tile_index % tiles_X;
	uint32_t Y = tile_index / tiles_X;
	uint32_t tile_size = cached_config.TileSize;

	uint32_t tile_w = std::min(tile_size, width - X * tile_size);
	uint32_t tile_h = std::min(tile_size, height - Y * tile_size);

	uint32_t x_offset = X * tile_size;
	uint32_t y_offset = Y * tile_size;

	upload_tile(subbuf_index, x_offset, y_offset, tile_w, tile_h);
}
#else
void Pathtracer::raytrace_scene_to_buf() {

	if (cached_config.Multithreaded)
	{
		myn::ThreadSafeQueue<uint> tasks;
		uint task_size = cached_config.TileSize * cached_config.TileSize;
		uint image_size = width * height;
		for (uint i = 0; i < image_size; i += task_size) {
			tasks.enqueue(i);
		}
		std::function<void(int)> raytrace_task = [&](int tid){
			uint task_begin;
			while (tasks.dequeue(task_begin))
			{
				uint task_end = glm::min(image_size, task_begin + task_size);
				for (uint task = task_begin; task < task_end; task++)
				{
					vec3 color = raytrace_pixel(task);
					color = gamma_correct(color);
					set_mainbuffer_rgb(task, color);
				}
			}
		};
		TRACE("enqueued %zu tasks", tasks.size());
		// create the threads and execute
		std::vector<std::thread> threads_tmp;
		for (uint tid = 0; tid < cached_config.NumThreads; tid++) {
			threads_tmp.emplace_back(raytrace_task, tid);
		}
		TRACE("created %d threads", cached_config.NumThreads);
		for (uint tid = 0; tid < cached_config.NumThreads; tid++) {
			threads_tmp[tid].join();
		}
		TRACE("joined threads");
	}
	else
	{
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {

				uint32_t px_index = width * y + x;
				vec3 color = raytrace_pixel(px_index);
				set_mainbuffer_rgb(px_index, color);

			}
		}
	}

}

void Pathtracer::output_file(const std::string& path) {
	stbi_write_png(
		path.c_str(),
		width, height,4,
		image_buffer,
		width * 4);
}

// though render to file from GUI is not implemented yet...
void Pathtracer::render_to_file(const std::string& output_path_rel_to_bin)
{
#if GRAPHICS_DISPLAY
	if (!initialized) initialize();
#else
	initialize();
#endif

	double num_camera_rays = double(width * height * pixel_offsets.size()) * 1e-6;
	std::string workload = std::to_string((int)(num_camera_rays * 1000) * 0.001) + "M camera rays, "
		+ "max depth " + std::to_string(cached_config.MaxRayDepth) + ", "
		+ "RR threshold " + std::to_string((int)(cached_config.RussianRouletteThreshold * 100) * 0.01);

	uint32_t num_camera_rays_per_task = cached_config.TileSize * cached_config.TileSize * pixel_offsets.size();
	std::string threading = std::to_string(num_camera_rays_per_task) + " camera rays per tile, ";
	if (cached_config.Multithreaded) {
		threading += std::to_string(cached_config.NumThreads) + " threads";
	} else {
		threading += "single threaded";
	}

	TRACE("initialization complete. starting...\n\t%s\n\t%s", workload.c_str(), threading.c_str())
	TIMER_BEGIN
	raytrace_scene_to_buf();
	TIMER_END(duration)
	TRACE("done! took %f seconds", duration)

	output_file(output_path_rel_to_bin);
}
#endif