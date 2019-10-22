#include "bootloadercontroller.h"
#include <QDebug>
#include <QVBoxLayout>

BootloaderController::BootloaderController(const LoadConfig &conf, QObject *parent): QObject(parent)
{
    loader = new BootLoader(conf);
    loader->moveToThread(&loaderThread);

    connect(&loaderThread, &QThread::finished, loader, &QObject::deleteLater);
    connect(this, &BootloaderController::load, loader, &BootLoader::load);
    connect(loader, &BootLoader::loadComplete, this, &BootloaderController::loadComplete);
    connect(loader, &BootLoader::loadError, this, &BootloaderController::loadError);
    connect(loader, &BootLoader::info, this, &BootloaderController::info);
    connect(loader, &BootLoader::percentUpdate, this, &BootloaderController::percentUpdate);
    loaderThread.start();
}

void BootloaderController::updateConfig(const LoadConfig &conf)
{
    loader->updateConfig(conf);
}

BootloaderController::~BootloaderController()
{
    loader->stopLoad();
    loaderThread.quit();
    loaderThread.wait();
}
