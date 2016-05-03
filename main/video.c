#include <stdio.h>
#include <string.h>
#include <cmdline.h>
#include <input.h>
#include <list.h>
#include <log.h>
#include <video.h>

/* Command-line parameters */
static char *video_fe_name;
PARAM(video_fe_name, string, "video", NULL, "Selects video frontend")
static int scale = 1;
PARAM(scale, int, "scale", NULL, "Applies a screen scale ratio")

struct list_link *video_frontends;
static struct video_frontend *frontend;
static int width;
static int height;
static bool updated;

bool video_init(struct video_specs *vs)
{
	struct list_link *link = video_frontends;
	struct video_frontend *fe;
	window_t *window = NULL;

	if (frontend) {
		LOG_E("Video frontend already initialized!\n");
		return false;
	}

	/* Save dimensions */
	width = vs->width;
	height = vs->height;

	/* Validate video option */
	if (!video_fe_name) {
		LOG_W("No video frontend selected!\n");
		return true;
	}

	/* Validate scaling factor */
	if (scale <= 0) {
		LOG_E("Scaling factor should be positive!\n");
		return false;
	}

	/* Reset updated state */
	updated = false;

	/* Find video frontend */
	while ((fe = list_get_next(&link))) {
		/* Skip if name does not match */
		if (strcmp(video_fe_name, fe->name))
			continue;

		/* Initialize frontend */
		if (fe->init) {
			vs->scale = scale;
			window = fe->init(fe, vs);
			if (!window)
				return false;
		}

		/* Save frontend */
		frontend = fe;

		/* Initialize input frontend */
		return input_init(fe->input, window);
	}

	/* Warn as video frontend was not found */
	LOG_E("Video frontend \"%s\" not recognized!\n", video_fe_name);
	return false;
}

void video_update()
{
	/* Set updated state */
	updated = true;

	if (!frontend)
		return;

	if (frontend->update)
		frontend->update(frontend);

	/* Update input sub-system as well */
	input_update();
}

bool video_updated()
{
	bool ret;

	/* Get current state */
	ret = updated;

	/* Reset state if needed */
	if (updated)
		updated = false;

	/* Return old state */
	return ret;
}

void video_lock()
{
	if (frontend && frontend->lock)
		frontend->lock(frontend);
}

void video_unlock()
{
	if (frontend && frontend->unlock)
		frontend->unlock(frontend);
}

void video_get_size(int *w, int *h)
{
	*w = width;
	*h = height;
}

void video_set_size(int w, int h)
{
	window_t *window;

	width = w;
	height = h;

	if (frontend && frontend->set_size) {
		window = frontend->set_size(frontend, w, h);
		input_set_window(window);
	}
}

struct color video_get_pixel(int x, int y)
{
	struct color default_color = { 0, 0, 0 };
	if (frontend && frontend->get_p)
		return frontend->get_p(frontend, x, y);
	return default_color;
}

void video_set_pixel(int x, int y, struct color color)
{
	if (frontend && frontend->set_p)
		frontend->set_p(frontend, x, y, color);
}

void video_deinit()
{
	if (!frontend)
		return;

	if (frontend->deinit)
		frontend->deinit(frontend);
	input_deinit();
	frontend = NULL;
}

