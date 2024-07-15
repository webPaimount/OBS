#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("nv-filters", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "NVIDIA filters";
}

#ifdef LIBNVAFX_ENABLED
extern struct obs_source_info nvidia_audiofx_filter;
extern bool load_nvidia_afx(void);
extern void unload_nvidia_afx(void);
#endif
#ifdef LIBNVVFX_ENABLED
extern struct obs_source_info nvidia_greenscreen_filter_info;
extern bool load_nvidia_vfx(void);
extern void unload_nvidia_vfx(void);
#endif

bool obs_module_load(void)
{
#ifdef LIBNVAFX_ENABLED
	/* load nvidia audio fx dll */
	if (load_nvidia_afx())
		obs_register_source(&nvidia_audiofx_filter);
#endif
#ifdef LIBNVVFX_ENABLED
	if (load_nvidia_vfx())
		obs_register_source(&nvidia_greenscreen_filter_info);
#endif
	return true;
}

void obs_module_unload(void)
{
#ifdef LIBNVAFX_ENABLED
	unload_nvidia_afx();
#endif
#ifdef LIBNVVFX_ENABLED
	unload_nvidia_vfx();
#endif
}
