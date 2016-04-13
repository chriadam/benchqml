#include <QObject>
#include <QUrl>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSEngine>
#include <QQmlComponent>
#include <QQuickView>
#include <QQuickItem>
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QtDebug>

#include <valgrind/callgrind.h>

class BenchHarness : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged)

public:
    explicit BenchHarness(const QUrl &url, bool warmMode, QObject *parent)
        : QObject(parent), m_url(url), m_warm(warmMode), m_finalTrace(false), m_terminate(true) { }

    QUrl url() const { return m_url; }

public Q_SLOTS:
    void compile() {
        QQmlComponent c(&m_engine, this);

        if (m_warm) {
            // precompile
            QQmlComponent c(&m_engine, this);
            c.loadUrl(m_url);
        }

        m_timer.start();
        CALLGRIND_START_INSTRUMENTATION;
        c.loadUrl(m_url);
        addTrace(QStringLiteral("compilation completed"));
        CALLGRIND_STOP_INSTRUMENTATION;
        CALLGRIND_DUMP_STATS;
        addFinalTrace(QStringLiteral("finished"));
    }

    void instantiate() {
        QQmlComponent c(&m_engine, this);
        c.loadUrl(m_url);
        if (c.errors().size()) {
            qDebug() << "Component errors:" << c.errors();
            emit done();
            return;
        }

        if (m_warm) {
            // pre-instantiate
            QObject *o = c.create();
            o->deleteLater();
        }

        m_timer.start();
        CALLGRIND_START_INSTRUMENTATION;
        QObject *o = c.create();
        addTrace(QStringLiteral("instantiation completed"));
        CALLGRIND_STOP_INSTRUMENTATION;
        CALLGRIND_DUMP_STATS;
        addFinalTrace(QStringLiteral("finished"));
        o->deleteLater();
    }

    void compileinstantiate() {
        QQmlComponent c(&m_engine, this);

        if (m_warm) {
            // pre-compile and pre-instantiate
            c.loadUrl(m_url);
            QObject *o = c.create();
            o->deleteLater();
        }

        m_timer.start();
        CALLGRIND_START_INSTRUMENTATION;
        c.loadUrl(m_url);
        addTrace(QStringLiteral("compilation completed"));
        QObject *o = c.create();
        addTrace(QStringLiteral("instantiation completed"));
        CALLGRIND_STOP_INSTRUMENTATION;
        CALLGRIND_DUMP_STATS;
        addFinalTrace(QStringLiteral("finished"));
        o->deleteLater();
    }

    void render(bool terminateAfterFirstFrame = true) {
        m_terminate = terminateAfterFirstFrame;

        if (m_warm) {
            // pre-compile and pre-instantiate
            QQmlComponent c(&m_engine, this);
            c.loadUrl(m_url);
            QObject *o = c.create();
            o->deleteLater();
        }

        connect(&m_view, SIGNAL(sceneGraphInitialized()), this, SLOT(viewSgInitialized()), Qt::DirectConnection);
        connect(&m_view, SIGNAL(afterRendering()), this, SLOT(viewAfterRendering()), Qt::DirectConnection);
        connect(&m_view, SIGNAL(frameSwapped()), this, SLOT(viewFrameSwapped()), Qt::DirectConnection);
        m_timer.start();

        CALLGRIND_START_INSTRUMENTATION;
        m_view.setSource(m_url);
        addTrace(QStringLiteral("compilation+instantiation completed"));
        m_view.show();
        addTrace(QStringLiteral("view shown"));
    }

    void twoStage() {
        m_terminate = false;

        if (m_warm) {
            // pre-compile and pre-instantiate
            QQmlComponent c(&m_engine, this);
            c.loadUrl(m_url);
            QObject *o = c.create();
            o->deleteLater();
        }

        m_view.rootContext()->setContextProperty("Harness", this);
        m_view.setSource(QUrl("qrc:///shim.qml"));
        m_view.show();
    }
    void continueTwoStage(QQuickItem *root) {
        QQmlComponent c(&m_engine, root);
        m_timer.start();
        CALLGRIND_START_INSTRUMENTATION;
        addTrace(QStringLiteral("beginning second stage"));
        c.loadUrl(m_url);
        addTrace(QStringLiteral("compilation completed"));
        QObject *o = c.create();
        QQuickItem *i = qobject_cast<QQuickItem*>(o);
        addTrace(QStringLiteral("instantiation completed"));
        if (!o) { qWarning() << "Aborting benchmark: could not create component: " << m_url << ":" << c.errors(); emit done(); return; }
        if (!i) { qWarning() << "Aborting benchmark: root item in component: " << m_url << "must be QQuickItem / Item derived!"; emit done(); return; }
        i->setParentItem(root);
        addTrace(QStringLiteral("reparented item"));
        connect(&m_view, SIGNAL(sceneGraphInitialized()), this, SLOT(viewSgInitialized()), Qt::DirectConnection);
        connect(&m_view, SIGNAL(afterRendering()), this, SLOT(viewAfterRendering()), Qt::DirectConnection);
        connect(&m_view, SIGNAL(frameSwapped()), this, SLOT(viewFrameSwapped()), Qt::DirectConnection);
    }

    void viewSgInitialized() {
        addTrace(QStringLiteral("scenegraph initialized"));
    }
    void viewAfterRendering() {
        addTrace(QStringLiteral("after rendering"));
    }
    void viewFrameSwapped() {
        addTrace(QStringLiteral("frame swapped"));
        CALLGRIND_STOP_INSTRUMENTATION;
        CALLGRIND_DUMP_STATS;
        addFinalTrace("finished");
    }

Q_SIGNALS:
    void urlChanged();
    void finishedTraces();
    void done();

private:
    QUrl m_url;
    bool m_warm;
    QQmlEngine m_engine;
    QQuickView m_view;
    QElapsedTimer m_timer;

    QMutex m_protect;
    bool m_finalTrace;
    bool m_terminate;
    QMap<qint64, QString> m_nsElapsed;
    void addTrace(const QString &trace) {
        QMutexLocker lock(&m_protect);
        if (!m_finalTrace) {
            m_nsElapsed.insert(m_timer.nsecsElapsed(), trace);
        }
    }
    void addFinalTrace(const QString &trace) {
        QMutexLocker lock(&m_protect);
        if (!m_finalTrace) {
            m_finalTrace = true;
            m_nsElapsed.insert(m_timer.nsecsElapsed(), trace);
            emit finishedTraces();
        }
    }

public Q_SLOTS:
    void dumpTraces() {
        QMutexLocker lock(&m_protect);
        qint64 prev = 0;
        for (QMap<qint64, QString>::const_iterator it = m_nsElapsed.begin(); it != m_nsElapsed.end(); ++it) {
            qDebug() << it.value() << "at" << it.key() << "==" << (it.key()-prev) << "nsecs.";
            prev = it.key();
        }
        if (m_terminate) {
            emit done();
        }
    }
};
