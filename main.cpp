#include <QGuiApplication>
#include <QQmlEngine>
#include <QJSEngine>
#include <QQmlComponent>
#include <QObject>
#include <QStringList>

#include <QtDebug>

#include "benchharness.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QStringList args = app.arguments();
    QString binary = args.takeFirst();
    if (args.length() < 2 || args.length() > 4) {
        qDebug() << "usage:" << binary << "[-C|-I|-B|-R|-T] [-W] file.qml";
        qDebug() << "options:\n"
                    "    -C  compile-only (then terminate)\n"
                    "    -I  instantiate only (then terminate)\n"
                    "    -B  compile and instantiate (then terminate)\n"
                    "    -R  compile and instantiate and then render first frame (then terminate; to avoid terminate omit R arg)\n"
                    "    -T  load shim Item, then when clicked will compile+instantiate+render\n"
                    "    -W  warm mode, will compile+instantiate object twice, but only produce timings/callgrind-output/render the second run\n";
        qDebug() << "examples:\n"
                    "    - to collect timing data for compilation of MyButton.qml:\n"
                    "    -> ./benchqml -C MyButton.qml\n"
                    "    - to collect callgrind information for warm instantiation of MyButton.qml:\n"
                    "    -> valgrind --tool=callgrind --separate-callers=3 --separate-recs=8 --separate-threads=yes --cache-sim=yes --instr-atstart=no ./benchqml -I -W MyButton.qml\n"
                    "    - to delay the collection of callgrind statistics or timing data or QML Analyzer events until mouseclick-event, via two-stage (T) mode:\n"
                    "    -> ./benchqml -T MyButton.qml [loads and displays a shim Item first, clicking in it will trigger C+I+R of MyButton.qml]\n";
        return 0;
    }

    QString url = args.takeLast();
    if (!url.endsWith(QLatin1String(".qml"), Qt::CaseInsensitive)) {
        qWarning() << "Invalid argument: file not a .qml file:" << url;
        return 0;
    }

    bool terminateRender = false;
    bool warm = false;
    QString warmOpt;
    QString typeOpt;
    if (!args.length()) {
        typeOpt = QLatin1String("R"); // default
        terminateRender = false; // no typeopt specified, don't terminate R mode.
    } else {
        if (args.length() == 2) {
            // typeopt warmopt
            warmOpt = args.takeLast();
            typeOpt = args.takeLast();
            terminateRender = true; // typeopt specified = terminate R mode.
        } else if (args.length() == 1) {
            // typeopt OR warmopt
            typeOpt = args.takeLast();
            if (typeOpt.compare(QLatin1String("W"),  Qt::CaseInsensitive) == 0 ||
                typeOpt.compare(QLatin1String("-W"), Qt::CaseInsensitive) == 0) {
                warmOpt = typeOpt;
                typeOpt = QLatin1String("R"); // default
                terminateRender = false; // no typeopt specified, don't terminate R mode.
            } else {
                terminateRender = true; // typeopt specified = terminate R mode.
            }
        }

        // check our options
        if (warmOpt.length()) {
            if (warmOpt.compare(QLatin1String("W"),  Qt::CaseInsensitive) != 0 &&
                warmOpt.compare(QLatin1String("-W"), Qt::CaseInsensitive) != 0) {
                qWarning() << "Invalid argument: warm switch unknown:" << warmOpt;
                return 0;
            }
            warm = true;
        }
    }

    BenchHarness h(QUrl(url), warm, Q_NULLPTR);
    QObject::connect(&h, SIGNAL(finishedTraces()), &h, SLOT(dumpTraces()), Qt::QueuedConnection);
    QObject::connect(&h, SIGNAL(done()), &app, SLOT(quit()), Qt::QueuedConnection);

    if (typeOpt.compare(QLatin1String("C"),  Qt::CaseInsensitive) == 0
            || typeOpt.compare(QLatin1String("-C"), Qt::CaseInsensitive) == 0) {
        qDebug() << "Producing trace of compilation.";
        h.compile();
    } else if (typeOpt.compare(QLatin1String("I"),  Qt::CaseInsensitive) == 0
            || typeOpt.compare(QLatin1String("-I"), Qt::CaseInsensitive) == 0) {
        qDebug() << "Producing trace of instantiation.";
        h.instantiate();
    } else if (typeOpt.compare(QLatin1String("B"),  Qt::CaseInsensitive) == 0
            || typeOpt.compare(QLatin1String("-B"), Qt::CaseInsensitive) == 0) {
        qDebug() << "Producing trace of compilation + instantiation.";
        h.compileinstantiate();
    } else if (typeOpt.compare(QLatin1String("R"),  Qt::CaseInsensitive) == 0
            || typeOpt.compare(QLatin1String("-R"), Qt::CaseInsensitive) == 0) {
        qDebug() << "Producing trace of compilation + instantiation + render; terminate after first frame =" << terminateRender;
        h.render(terminateRender);
    } else if (typeOpt.compare(QLatin1String("T"),  Qt::CaseInsensitive) == 0
            || typeOpt.compare(QLatin1String("-T"), Qt::CaseInsensitive) == 0) {
        qDebug() << "Producing two-stage trace of compilation + instantiation + render with no termination, useful when using QML Analyzer.\nClick anywhere in the initial view to trigger compile+instantiation+reparent of the specified QtQuick component.";
        h.twoStage();
    } else {
        qWarning() << "Invalid argument: switch unknown:" << typeOpt;
        return 0;
    }

    return app.exec();
}
