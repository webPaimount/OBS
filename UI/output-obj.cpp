#include "output-obj.hpp"
#include "obs-frontend-api.h"

static void onEvent(enum obs_frontend_event event, void *param)
{
	OutputObj *obj = static_cast<OutputObj *>(param);

	OBSSourceAutoRelease source;

	switch (event) {
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		source = obs_frontend_get_current_preview_scene();
		break;
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		source = obs_frontend_get_current_scene();
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		if (!obs_frontend_preview_program_mode_active())
			source = obs_frontend_get_current_scene();

		break;
	default:
		break;
	}

	obj->SetSource(source.Get());
}

OutputObj::OutputObj(const char *id, OBSData settings, enum OutputType type_)
	: type(type_)
{
	output = obs_output_create(id, id, settings, nullptr);
}

OutputObj::~OutputObj()
{
	Stop();
}

OBSOutput OutputObj::GetOutput()
{
	return output.Get();
}

enum OutputType OutputObj::GetType()
{
	return type;
}

void OutputObj::SetType(enum OutputType type_)
{
	if (type == type_)
		return;

	type = type_;

	if (Active()) {
		Stop();
		Start();
	}
}

bool OutputObj::Active()
{
	return active;
}

bool OutputObj::Start()
{
	if (Active())
		return false;

	SetType(type);

	if (type == OutputType::Program) {
		video = obs_get_video();
	} else {
		OBSSourceAutoRelease source;

		if (type == OutputType::Preview) {
			obs_frontend_add_event_callback(onEvent, this);

			if (obs_frontend_preview_program_mode_active())
				source =
					obs_frontend_get_current_preview_scene();
			else
				source = obs_frontend_get_current_scene();
		}

		view = obs_view_create();
		video = obs_view_add(view);
		obs_view_set_source(view, 0, source);
	}

	obs_output_set_media(output, video, obs_get_audio());

	active = true;
	bool success = obs_output_start(output);

	if (!success)
		Stop();

	return success;
}

void OutputObj::Stop()
{
	if (!Active())
		return;

	active = false;

	if (type == OutputType::Preview)
		obs_frontend_remove_event_callback(onEvent, this);

	obs_output_stop(output);
	obs_output_set_media(output, nullptr, nullptr);

	if (view) {
		obs_view_remove(view);
		obs_view_set_source(view, 0, nullptr);
		obs_view_destroy(view);
		view = nullptr;
	}

	video = nullptr;
}

void OutputObj::SetSource(OBSSource source)
{
	if (source != GetSource()) {
		weakSource = OBSGetWeakRef(source);

		if (view)
			obs_view_set_source(view, 0, source);
	}
}

OBSSource OutputObj::GetSource()
{
	return OBSGetStrongRef(weakSource);
}

void OutputObj::UpdateSettings(OBSData settings)
{
	obs_output_update(output, settings);
}
