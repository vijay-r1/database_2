#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QUrl>
#include <QtQuickControls2/QQuickStyle>

#include "datafetcher.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);

    qmlRegisterType<DataFetcher>("com.thermo.fetcher", 1, 0, "DataFetcher");

    QQmlApplicationEngine engine;

    DataFetcher dataFetcher;
    engine.rootContext()->setContextProperty("dataFetcher", &dataFetcher);


    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
