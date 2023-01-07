#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "Qrimageprovider.hpp"



int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);

	QQmlApplicationEngine engine;
	engine.addImageProvider(QLatin1String("qrcodeblack"), new QRImageProvider("black",1));

	const QUrl url(u"qrc:/binter/binter/qml/window_server.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                            &app, [url](QObject *obj, const QUrl &objUrl) {
                            if (!obj && url == objUrl)
                            QCoreApplication::exit(-1);
                            }, Qt::QueuedConnection);

	engine.load(url);


	return app.exec();
}
