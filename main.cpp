#include "VideoView.h"
#include <QApplication>
#include <QFile>

#include <QQuickView>
#include <QQmlContext>
#include <QSettings>
#include <QBuffer>
#include <QScreen>

#include "VideoThread.h"

#include "FboRenderer.h"

#include "FeedOutput.h"

#include "StdInThread.h"

#include <QtAV>

QQuickView *view = NULL;

QStringList formatList()
{
    QStringList formatlist;

    formatlist.append("MPEG-4 (mp4)");
    formatlist.append("Matroska (mkv)");
    formatlist.append("Flash Video (flv)");
    formatlist.append("QuickTime (mov)");

    return formatlist;
}

QStringList qualityList()
{
    QStringList qualitylist;

    qualitylist.append("High");
    qualitylist.append("Medium");
    qualitylist.append("Low");
    qualitylist.append("Stream");

    return qualitylist;
}

QVector<QtAV::VideoDecoderId> decoderIds()
{
    QVector<QtAV::VideoDecoderId> ids;

    QStringList valid_names;

    QSettings settings(QApplication::applicationDirPath() + "/quality.ini", QSettings::IniFormat);

#ifdef _WIN32
    valid_names << "CUDA";
    valid_names << "DXVA";
    valid_names << "D3D11";
    settings.beginGroup("decoders_windows");
#else
    valid_names << "VDA";
    valid_names << "VTBox";
    settings.beginGroup("decoders_osx");
#endif
    valid_names << "FFmpeg";

    for(int i=0; i<10; i++) {
        QString name = settings.value(QString::number(i)).toString();
        if(valid_names.contains(name))
            ids << QtAV::VideoDecoder::id(name.toLatin1().constData());
    }

    return ids;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Delicode");
    QCoreApplication::setOrganizationDomain("delicode.com");
    QCoreApplication::setApplicationName("Vidiot");

    qmlRegisterType<VideoView>("videoview", 1, 0, "VideoView");
    qRegisterMetaType<RecorderFrame>("RecorderFrame");

    QString input;
    QString output;
    QString quality = "Medium";

    if(argc > 1) {
        if(QString(argv[1]) == "extract_thumbnail") {
            if(argc != 5)
                return 0;

            int w = atoi(argv[2]);
            int h = atoi(argv[3]);

            if(w <= 0 || w > 1000 || h <= 0 || h > 1000)
                return 0;

            VideoPlayer p(argv[4]);
            p.seeking = true;
            p.next_seek_time_s = p.duration/3.f;
            p.play();
            if(p.width > 0 && p.height > 0 && p.data) {
                QImage img(p.width, p.height, QImage::Format_RGB888);
                memcpy(img.bits(), p.data, p.width*p.height*3*sizeof(unsigned char));

                QByteArray ba;
                QBuffer buffer(&ba);
                buffer.open(QIODevice::WriteOnly);
                img.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buffer, "JPG");
                buffer.close();

                cout << "length:" << p.duration << ":thumbnail:" << ba.toBase64().data() << endl;
            }
            return 0;
        }
        else if(QString(argv[1]) == "list_devices") {
            QStringList devices = CaptureProcessor::listDevices();
            for(int i=0; i<devices.length(); i++)
                cout << devices.at(i).toLocal8Bit().constData() << endl;

            return 0;
        }
        else if(QString(argv[1]) == "play") {
            if(argc == 4) {
                input = argv[2];
                output = argv[3];
            }
            else
                return 0;
        }
        else if(QString(argv[1]) == "record") {
            if(argc > 3) {
                input = argv[2];
                output = argv[3];
                if(argc > 4)
                    quality = argv[4];
            }
        }
    }

    QtAV::AVPlayer plr;
    QtAV::AVPlayer *player = NULL;

    QtAV::FboRenderer fr;

    if(output.startsWith("feed:///"))
        FeedOutput::instance().setName(output);

    VideoRecorder::list_codecs(true);

    CaptureThread thread;
    RecordThread rthread;
    StdInThread inthread;

    QObject::connect(&thread.processor, SIGNAL(newRecorderFrame(RecorderFrame)), &rthread.controller, SLOT(recorderFrame(RecorderFrame)));
    QObject::connect(&thread.processor, SIGNAL(stopRecording()), &rthread.controller, SLOT(stop()));


    if(input.isEmpty()) {
        view = new QQuickView();
        view->setTitle("Vidiöt");

        view->setIcon(QIcon(":/vidiot_icon.png"));

        thread.processor.setMainView(view);
        view->rootContext()->setContextProperty("formatlist", formatList());
        view->rootContext()->setContextProperty("qualitylist", qualityList());

        QSettings settings;
        view->rootContext()->setContextProperty("initial_format", settings.value("format", "MPEG-4 (mp4)").toString());
        view->rootContext()->setContextProperty("initial_quality", settings.value("quality", "Medium").toString());

        view->rootContext()->setContextProperty("dpi", qApp->primaryScreen()->logicalDotsPerInch()/96.f);

        view->setSource(QUrl("qrc:/ui.qml"));

        QObject *object = view->rootObject();

        player = object->findChild<QtAV::AVPlayer*>();

        VideoView *videoview = object->findChild<VideoView*>();

        QObject::connect(&fr, SIGNAL(sendFeed(unsigned int, int, int)), &FeedOutput::instance(), SLOT(send(uint,int,int)));
        QObject::connect(&thread.processor, SIGNAL(sendFeed(uint,int,int,bool)), &FeedOutput::instance(), SLOT(send(uint,int,int,bool)));

        QObject::connect(videoview, SIGNAL(setResolution(QString)), &FeedOutput::instance(), SLOT(setResolution(QString)));
        QObject::connect(&fr, SIGNAL(sendFeed(unsigned int, int, int)), videoview, SLOT(draw(uint,int,int)));
        QObject::connect(&thread.processor, SIGNAL(sendFeed(uint,int,int,bool)), videoview, SLOT(draw(uint,int,int,bool)));
        QObject::connect(&thread.processor, SIGNAL(error(QString)), videoview, SLOT(receiveError(QString)));

        QObject::connect(&fr, SIGNAL(sendResolution(int,int)), &thread.processor, SLOT(updateResolutionList(int,int)));
        QObject::connect(&thread.processor, SIGNAL(sendResolution(int,int)), &thread.processor, SLOT(updateResolutionList(int,int)));

        QObject::connect(videoview, SIGNAL(setSource(QString)), &thread.processor, SLOT(setSource(QString)));
        QObject::connect(videoview, SIGNAL(setResolution(QString)), &thread.processor, SLOT(setResolution(QString)));
        QObject::connect(videoview, SIGNAL(action(QString)), &thread.processor, SLOT(action(QString)));
        QObject::connect(videoview, SIGNAL(updateSources()), &thread.processor, SLOT(updateSourceList()));

        QObject::connect(videoview, SIGNAL(setFormat(QString)), &rthread.controller, SLOT(setFormat(QString)));
        QObject::connect(videoview, SIGNAL(setQuality(QString)), &rthread.controller, SLOT(setQuality(QString)));

        QObject::connect(videoview, SIGNAL(record(bool)), &thread.processor, SLOT(record(bool)));

        QObject::connect(videoview, SIGNAL(setResolution(QString)), &fr, SLOT(setResolution(QString)));

        QObject::connect(&thread.processor, SIGNAL(updateFPS(float)), videoview, SLOT(setFps(float)));
        QObject::connect(&thread.processor, SIGNAL(updateSleeptime(float)), videoview, SLOT(setSleeptime(float)));
        QObject::connect(&thread.processor, SIGNAL(updateRecordtime(float)), videoview, SLOT(setRecordtime(float)));

        QObject::connect(&fr, SIGNAL(updateFPS(float)), videoview, SLOT(setFps(float)));

        QObject::connect(videoview, SIGNAL(showCameraProperties()), &thread.processor, SLOT(cameraPropertiesRequested()));

        thread.processor.setVideoView(videoview);
    }
    else if(input.startsWith("dshow:///")) {
        QObject::connect(&fr, SIGNAL(sendFeed(unsigned int, int, int)), &FeedOutput::instance(), SLOT(send(uint,int,int)));
        QObject::connect(&thread.processor, SIGNAL(sendFeed(uint,int,int,bool)), &FeedOutput::instance(), SLOT(send(uint,int,int,bool)));

        thread.processor.setSource(input);
        thread.processor.setPingRequired(true);

        QObject::connect(&inthread.listener, SIGNAL(ping()), &thread.processor, SLOT(ping()));
        QObject::connect(&inthread.listener, SIGNAL(stop()), &thread.processor, SLOT(stop()));

        QObject::connect(&inthread.listener, SIGNAL(showCameraProperties()), &thread.processor, SLOT(cameraPropertiesRequested()));

        inthread.start();
    }
    else if(input.startsWith("feed:///")) {
        thread.processor.setSource(input);
        thread.processor.setPingRequired(true);

        QObject::connect(&inthread.listener, SIGNAL(ping()), &thread.processor, SLOT(ping()));
        QObject::connect(&inthread.listener, SIGNAL(stop()), &thread.processor, SLOT(stop()));
        inthread.start();

        if(output.startsWith("file:///"))
            output.remove("file:///");
        rthread.controller.setFilename(output);
        rthread.controller.setQuality(quality);
    }
    else {
        QObject::connect(&fr, SIGNAL(sendFeed(unsigned int, int, int)), &FeedOutput::instance(), SLOT(send(uint,int,int)));
        QObject::connect(&thread.processor, SIGNAL(sendFeed(uint,int,int,bool)), &FeedOutput::instance(), SLOT(send(uint,int,int,bool)));

        player = &plr;

        thread.processor.setPingRequired(true);

        QObject::connect(&inthread.listener, SIGNAL(pause(bool)), player, SLOT(pause(bool)));
        QObject::connect(&inthread.listener, SIGNAL(seek(qreal)), player, SLOT(seek(qreal)));

        QObject::connect(&inthread.listener, SIGNAL(ping()), &thread.processor, SLOT(ping()));
        QObject::connect(&inthread.listener, SIGNAL(stop()), &thread.processor, SLOT(stop()));

        QObject::connect(player, SIGNAL(durationChanged(qint64)), &thread.processor, SLOT(setDuration(qint64)));
        QObject::connect(player, SIGNAL(positionChanged(qint64)), &thread.processor, SLOT(setPosition(qint64)));

        inthread.start();
    }

    if(player) {
        player->setPriority(decoderIds());
        player->addVideoRenderer(&fr);

        if(!input.isEmpty()) {
            player->play(input);
            player->audio()->setVolume(0);
            player->audio()->setMute(true);
        }
    }

    rthread.start();
    thread.start();

    if(view) {
        view->setGeometry(100, 100, 640, 480);
        view->show();
    }

    int ret = a.exec();

    std::cout << "returned from main event loop" << std::endl;

    thread.quit();
    thread.wait(10000);

    std::cout << "waited for processing thread" << std::endl;

    rthread.quit();
    rthread.wait(1000);

    std::cout << "waited for rendering thread" << std::endl;

    int remaining_now = rthread.controller.framesRemaining();
    int remaining_before = remaining_now+1;

    while(remaining_now > 0 && remaining_now < remaining_before) {
        rthread.wait(1000);
        std::cout << "waited for rendering thread again" << std::endl;
        remaining_before = remaining_now;
        remaining_now = rthread.controller.framesRemaining();
    }

    std::cout << "rendering thread wait over" << std::endl;

    if(view)
        delete view;

    return ret;
}