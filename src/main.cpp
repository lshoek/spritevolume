// Local Includes
#include "spritevolumeapp.h"

// Nap includes
#include <apprunner.h>
#include <nap/logger.h>
#include <guiappeventhandler.h>

int main(int argc, char *argv[])
{
    nap::Core core;

    nap::AppRunner<nap::SpriteVolumeApp, nap::GUIAppEventHandler> app_runner(core);
	app_runner.getApp().setFilename("app.json");

    nap::utility::ErrorState error;
    if (!app_runner.start(error))
    {
        nap::Logger::fatal("error: %s", error.toString().c_str());
        return -1;
    }
    return app_runner.exitCode();
}
                                         