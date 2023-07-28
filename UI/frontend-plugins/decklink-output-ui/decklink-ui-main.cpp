#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <util/util.hpp>
#include <util/platform.h>
#include "DecklinkOutputUI.h"
#include "../../../plugins/decklink/const.h"
#include "../../output-obj.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("decklink-output-ui", "en-US")

static DecklinkOutputUI *doUI = nullptr;
static OutputObj *output = nullptr;
static OutputObj *preview = nullptr;

OBSData load_settings(const char *filename)
{
	BPtr<char> path =
		obs_module_get_config_path(obs_current_module(), filename);
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		OBSDataAutoRelease data = obs_data_create_from_json(jsonData);
		return data.Get();
	}

	return nullptr;
}

void output_stop()
{
	output->Stop();
	doUI->OutputStateChanged(false);
}

void output_start()
{
	OBSData settings = load_settings("decklinkOutputProps.json");

	if (settings) {
		output->UpdateSettings(settings);

		bool started = output->Start();
		doUI->OutputStateChanged(started);

		if (!started)
			output_stop();
	}
}

void output_toggle()
{
	if (output->Active())
		output_stop();
	else
		output_start();
}

void preview_output_stop()
{
	preview->Stop();
	doUI->PreviewOutputStateChanged(false);
}

void preview_output_start()
{
	OBSData settings = load_settings("decklinkPreviewOutputProps.json");

	if (settings) {
		preview->UpdateSettings(settings);

		bool started = preview->Start();
		doUI->PreviewOutputStateChanged(started);

		if (!started)
			preview_output_stop();
	}
}

void preview_output_toggle()
{
	if (preview->Active())
		preview_output_stop();
	else
		preview_output_start();
}

void addOutputUI(void)
{
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(
		obs_module_text("Decklink Output"));

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();

	obs_frontend_push_ui_translation(obs_module_get_string);
	doUI = new DecklinkOutputUI(window);
	obs_frontend_pop_ui_translation();

	auto cb = []() {
		doUI->ShowHideDialog();
	};

	action->connect(action, &QAction::triggered, cb);
}

static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		OBSData settings = load_settings("decklinkOutputProps.json");

		if (settings && obs_data_get_bool(settings, "auto_start"))
			output_start();

		OBSData previewSettings =
			load_settings("decklinkPreviewOutputProps.json");

		if (previewSettings &&
		    obs_data_get_bool(previewSettings, "auto_start"))
			preview_output_start();
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		output_stop();
		preview_output_stop();

		delete output;
		output = nullptr;

		delete preview;
		preview = nullptr;
	}
}

bool obs_module_load(void)
{
	return true;
}

void obs_module_post_load(void)
{
	if (!obs_get_module("decklink"))
		return;

	addOutputUI();

	obs_frontend_add_event_callback(OBSEvent, nullptr);

	OBSData settings = load_settings("decklinkOutputProps.json");
	output =
		new OutputObj("decklink_output", settings, OutputType::Program);

	settings = load_settings("decklinkPreviewOutputProps.json");
	preview =
		new OutputObj("decklink_output", settings, OutputType::Preview);
}
