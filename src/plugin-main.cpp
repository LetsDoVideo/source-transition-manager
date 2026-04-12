#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include "SourceTransitionDock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("source-transition-manager", "en-US")

static SourceTransitionDock *stmDock = nullptr;

bool obs_module_load(void)
{
    obs_log(LOG_INFO, "Source Transition Manager loaded");

    obs_frontend_push_ui_translation(obs_module_get_string);

    auto *mainWindow = static_cast<QMainWindow *>(
        obs_frontend_get_main_window());

    stmDock = new SourceTransitionDock(mainWindow);

    obs_frontend_add_dock_by_id(
        "source-transition-manager-dock",
        "Source Transition Manager",
        stmDock);

    return true;
}

void obs_module_unload(void)
{
    obs_log(LOG_INFO, "Source Transition Manager unloaded");
}
