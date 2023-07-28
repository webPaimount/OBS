#pragma once

#include "obs.hpp"

enum class OutputType {
	Unknown,
	Preview,
	Program,
	Source,
	Multiview, // TODO
};

class OutputObj {
	bool active = false;

	enum OutputType type;

	OBSOutputAutoRelease output;
	OBSWeakSourceAutoRelease weakSource;

	obs_view_t *view = nullptr;
	video_t *video = nullptr;

public:
	OBSOutput GetOutput();

	enum OutputType GetType();
	void SetType(enum OutputType type);

	bool Active();

	bool Start();
	void Stop();

	void SetSource(OBSSource source);
	OBSSource GetSource();

	void UpdateSettings(OBSData settings);

	OutputObj(const char *id, OBSData settings, enum OutputType type);
	~OutputObj();
};
