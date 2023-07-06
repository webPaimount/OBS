// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>

#include "twitch-api.hpp"

#ifdef OAUTH_ENABLED
#include <unordered_map>
#include <memory>
#include <obs.hpp>

#include "twitch-oauth.hpp"
#endif

class TwitchService {
	obs_service_info info = {0};

	std::vector<TwitchApi::Ingest> ingests;

	static void InfoFreeTypeData(void *typeData);

	static const char *InfoGetName(void *typeData);
	static void *InfoCreate(obs_data_t *settings, obs_service_t *service);
	static void InfoDestroy(void *data);
	static void InfoUpdate(void *data, obs_data_t *settings);

	static const char *InfoGetConnectInfo(void *data, uint32_t type);

	static const char *InfoGetProtocol(void *data);

	static const char **InfoGetSupportedVideoCodecs(void *data);

	static bool InfoCanTryToConnect(void *data);

	static int InfoGetMaxCodecBitrate(void *data, const char *codec);

	static obs_properties_t *InfoGetProperties(void *data);

	static bool InfoCanBandwidthTest(void *data);
	static void InfoEnableBandwidthTest(void *data, bool enabled);
	static bool InfoBandwidthTestEnabled(void *data);

#ifdef OAUTH_ENABLED
	OBSDataAutoRelease data = nullptr;

	std::unordered_map<std::string, std::shared_ptr<TwitchApi::ServiceOAuth>>
		oauths;
	std::unordered_map<std::string, unsigned int> oauthsRefCounter;
	bool deferUiFunction = true;

	void SaveOAuthsData();
	static void OBSEvent(obs_frontend_event event, void *priv);
#endif
public:
	TwitchService();
	~TwitchService() {}

	static void Register();

	std::vector<TwitchApi::Ingest> GetIngests(bool refresh = false);

#ifdef OAUTH_ENABLED
	TwitchApi::ServiceOAuth *GetOAuth(const std::string &uuid,
					  obs_service_t *service);
	void ReleaseOAuth(const std::string &uuid, obs_service_t *service);
#endif
};
