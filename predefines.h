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

#ifndef PREDEFINES_H
#define PREDEFINES_H

#include <QObject>
#include <QtCore/QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QPainter>
#include <QTimer>
#include <QElapsedTimer>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QSettings>
#include <QtConcurrentRun>
#include <QBuffer>
#include <QScreen>

#include <QQmlContext>
#include <QQuickView>
#include <QQuickFramebufferObject>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include <iostream>
#include <math.h>

#include <QtAV>

#define GLER {GLenum er = glGetError(); if(er != GL_NO_ERROR) qDebug() << "GLerror" << __FILE__ << ", " << __LINE__ << er;}

#endif // PREDEFINES_H
