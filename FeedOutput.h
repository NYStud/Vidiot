/*
    Vidiot id a VIDeo Input Output Transformer With a Touch of Functionality
    Copyright (C) 2016  Delicode Ltd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FEED_OUTPUT_H
#define FEED_OUTPUT_H

#include "predefines.h"

#ifdef _WIN32
#include "external/Spout/Spout.h"
#endif
#ifdef __APPLE__
#include "DelicodeSyphonServer.h"
#endif

class FeedOutput : public QObject
{
    Q_OBJECT
public:
    static FeedOutput &instance() {
        static FeedOutput inst;

        return inst;
    }

    QString feedname;

    void ensure(int width, int height);

    void setName(QString name) {
        if(name.startsWith("feed:///"))
            name.remove("feed:///");

        qDebug() << "Output name set to" << name;

        out_name = name;
    }

    QStringList listSources(bool list_self = false);

public slots:
    void sendTexture(unsigned int texid, int width = -1, int height = -1, bool flip = true);
    void sendFBO(unsigned int fboid, int width = -1, int height = -1, bool flip = true);
    void setResolution(QString resolution_str) {resolution = resolution_str;}

private:
    FeedOutput() : QObject(),
        created(false),
        w(0),
        h(0),
        gl(NULL),
        fbo(NULL)
    {
        gl = new QOpenGLWidget();
        gl->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint|Qt::Tool);
        gl->setGeometry(0,0,4,4);
        gl->show();
        QApplication::processEvents();
        gl->hide();
        QApplication::processEvents();
#ifdef __APPLE__
        syphonserver = NULL;
#endif
        setObjectName("FeedOutput");
    }

    ~FeedOutput() {
        if(gl)
            delete gl;
#ifdef __APPLE__
        if (syphonserver)
            delete syphonserver;
#endif
    }

    /* don't allow copying */
    FeedOutput(FeedOutput const&);
    void operator=(FeedOutput const&);

private:
    bool created;
    int w, h;
#ifdef _WIN32
    SpoutSender spoutserv;
#endif
#ifdef __APPLE__
    DelicodeSyphonServer* syphonserver;
#endif
    QString out_name;

    QOpenGLWidget *gl;
    QOpenGLFramebufferObject *fbo;

    QString resolution;

};

#endif //FEED_OUTPUT_H
