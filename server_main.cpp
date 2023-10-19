#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "Qrimageprovider.hpp"
#include "mydesigns.hpp"


int main(int argc, char *argv[])
{
	auto foo=fooDesign::fooPrint(); //https://forum.qt.io/post/762513
	foo=fooQtQrGen::fooPrint();

	QGuiApplication app(argc, argv);

	QQmlApplicationEngine engine;
	engine.addImportPath("qrc:/esterVtech.com/imports");
	engine.addImageProvider(QLatin1String("qrcodeblack"), new QRImageProvider("black",1));
	qDebug()<<engine.importPathList();
	qmlRegisterSingletonType(QUrl(u"qrc:/esterVtech.com/imports/MyDesigns/qml/CustomStyle.qml"_qs), "CustomStyle", 1, 0, "CustomStyle");
	const QUrl url(u"qrc:/esterVtech.com/imports/server/qml/window.qml"_qs);
	engine.load(url);
	return app.exec();
}
