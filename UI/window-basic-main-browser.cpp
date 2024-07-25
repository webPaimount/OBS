/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QDir>
#include <QThread>
#include <QMessageBox>
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#include <random>

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#include "window-dock-browser.hpp"
#endif

struct QCef;
struct QCefCookieManager;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

static std::string GenId()
{
	std::random_device rd;
	std::mt19937_64 e2(rd());
	std::uniform_int_distribution<uint64_t> dist(0, 0xFFFFFFFFFFFFFFFF);

	uint64_t id = dist(e2);

	char id_str[20];
	snprintf(id_str, sizeof(id_str), "%16llX", (unsigned long long)id);
	return std::string(id_str);
}

void CheckExistingCookieId()
{
	OBSBasic *main = OBSBasic::Get();
	if (config_has_user_value(main->Config(), "Panels", "CookieId"))
		return;

	config_set_string(main->Config(), "Panels", "CookieId",
			  GenId().c_str());
}

#ifdef BROWSER_AVAILABLE
static void InitPanelCookieManager()
{
	if (!cef)
		return;
	if (panel_cookies)
		return;

	CheckExistingCookieId();

	OBSBasic *main = OBSBasic::Get();
	const char *cookie_id =
		config_get_string(main->Config(), "Panels", "CookieId");

	std::string sub_path;
	sub_path += "obs_profile_cookies/";
	sub_path += cookie_id;

	panel_cookies = cef->create_cookie_manager(sub_path);
}
#endif

void DestroyPanelCookieManager()
{
#ifdef BROWSER_AVAILABLE
	if (panel_cookies) {
		panel_cookies->FlushStore();
		delete panel_cookies;
		panel_cookies = nullptr;
	}
#endif
}

void DeleteCookies()
{
#ifdef BROWSER_AVAILABLE
	if (panel_cookies) {
		panel_cookies->DeleteCookies("", "");
	}
#endif
}

void DuplicateCurrentCookieProfile(ConfigFile &config)
{
#ifdef BROWSER_AVAILABLE
	if (cef) {
		OBSBasic *main = OBSBasic::Get();
		std::string cookie_id =
			config_get_string(main->Config(), "Panels", "CookieId");

		std::string src_path;
		src_path += "obs_profile_cookies/";
		src_path += cookie_id;

		std::string new_id = GenId();

		std::string dst_path;
		dst_path += "obs_profile_cookies/";
		dst_path += new_id;

		BPtr<char> src_path_full = cef->get_cookie_path(src_path);
		BPtr<char> dst_path_full = cef->get_cookie_path(dst_path);

		QDir srcDir(src_path_full.Get());
		QDir dstDir(dst_path_full.Get());

		if (srcDir.exists()) {
			if (!dstDir.exists())
				dstDir.mkdir(dst_path_full.Get());

			QStringList files = srcDir.entryList(QDir::Files);
			for (const QString &file : files) {
				QString src = QString(src_path_full);
				QString dst = QString(dst_path_full);
				src += QDir::separator() + file;
				dst += QDir::separator() + file;
				QFile::copy(src, dst);
			}
		}

		config_set_string(config, "Panels", "CookieId",
				  cookie_id.c_str());
		config_set_string(main->Config(), "Panels", "CookieId",
				  new_id.c_str());
	}
#else
	UNUSED_PARAMETER(config);
#endif
}

void OBSBasic::InitBrowserPanelSafeBlock()
{
#ifdef BROWSER_AVAILABLE
	if (!cef)
		return;
	if (cef->init_browser()) {
		InitPanelCookieManager();
		return;
	}

	ExecThreadedWithoutBlocking([] { cef->wait_for_browser_init(); },
				    QTStr("BrowserPanelInit.Title"),
				    QTStr("BrowserPanelInit.Text"));
	InitPanelCookieManager();
#endif
}

#ifdef BROWSER_AVAILABLE
bool OBSBasic::IsBrowserInitialised()
{
	return !!cef;
}

void OBSBasic::StorePluginBrowserDock(const PluginBrowserParams &params)
{
	pluginBrowserDockNames.push_back(params.id);
	preInitPluginBrowserDocks.push_back(params);
}

void OBSBasic::LoadStoredPluginBrowserDock()
{
	for (int i = 0; preInitPluginBrowserDocks.size() > i; i++)
		AddPluginBrowserDock(preInitPluginBrowserDocks[i]);

	preInitPluginBrowserDocks.clear();
}

void OBSBasic::AddPluginBrowserDock(const PluginBrowserParams &params)
{
	static int panel_version = -1;
	if (panel_version == -1) {
		panel_version = obs_browser_qcef_version();
	}

	BrowserDock *dock = new BrowserDock();
	dock->setObjectName(params.id);
	dock->resize(460, 600);
	dock->setMinimumSize(80, 80);
	dock->setWindowTitle(params.title);

	QCefWidget *browser =
		cef->create_widget(dock, QT_TO_UTF8(params.url), nullptr);
	if (browser && panel_version >= 1)
		browser->allowAllPopups(true);

	dock->SetWidget(browser);

	if (!params.startupScript.isEmpty())
		browser->setStartupScript(params.startupScript.toStdString());

	for (int i = 0; params.forcePopupUrls.size() > i; i++)
		cef->add_force_popup_url(params.forcePopupUrls[i].toStdString(),
					 dock);

	if (!pluginBrowserDockNames.contains(dock->objectName()))
		pluginBrowserDockNames.push_back(dock->objectName());
	AddDockWidget(dock, Qt::RightDockWidgetArea);

	dock->setFloating(true);
	dock->setVisible(false);
}

void OBSBasic::ChangePluginBrowserDockUrl(const char *id_, const char *url)
{
	QString id = QT_UTF8(id_);
	if (pluginBrowserDockNames.contains(id) &&
	    extraDockNames.contains(id)) {
		int idx = extraDockNames.indexOf(id);
		reinterpret_cast<BrowserDock *>(extraDocks[idx].data())
			->cefWidget->setURL(url);
	}
}
#endif
