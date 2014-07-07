/*
 * Copyright (C) 2014 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "multipartformdataparser_p.h"
#include "upload_p.h"
#include "common.h"

#include <QRegularExpression>

using namespace Cutelyst;

MultiPartFormDataParser::MultiPartFormDataParser(const QByteArray &contentType, QIODevice *body) :
    d_ptr(new MultiPartFormDataParserPrivate)
{
    Q_D(MultiPartFormDataParser);
    d->body = body;

    QRegularExpression re("boundary=([^\";]+)");
    QRegularExpressionMatch match = re.match(contentType);
    if (match.hasMatch()) {
        QByteArray boundary = "--" + match.captured(1).toLocal8Bit();
        d->boundary = qstrdup(boundary.data());
        d->boundaryLength = boundary.size();
    }
    qCDebug(CUTELYST_MULTIPART) << "Boudary:" << d->boundary << d->boundaryLength;
}

MultiPartFormDataParser::~MultiPartFormDataParser()
{
    Q_D(MultiPartFormDataParser);
    delete [] d->boundary;
    delete d;
}

void MultiPartFormDataParser::setBufferSize(int size)
{
    Q_D(MultiPartFormDataParser);
    d->bufferSize = qMax(size, 32);
}

Uploads MultiPartFormDataParser::parse()
{
    Q_D(MultiPartFormDataParser);
    if (!d->boundaryLength) {
        return Uploads();
    }

    Uploads ret;
    char *buffer = new char[d->bufferSize];

    qint64 origPos = d->body->pos();
    d->body->seek(0);
    ret = d->execute(buffer);
    d->body->seek(origPos);

    delete [] buffer;
    return ret;
}

Uploads MultiPartFormDataParserPrivate::execute(char *buffer)
{
    Uploads ret;
    QByteArray header;
    Headers headers;
    qint64 startOffset;
    int boundaryPos = 0;
    ParserState state = FindBoundary;

    while (!body->atEnd()) {
        qint64 len = body->read(buffer, bufferSize);
        int i = 0;
        while (i < len) {
            switch (state) {
            case FindBoundary:
                i += findBoundary(buffer + i, len - i, state, boundaryPos);
                break;
            case EndBoundaryCR:
                // TODO the "--" case
                if (buffer[i] != '\r') {
                    qCDebug(CUTELYST_MULTIPART) << "EndBoundaryCR return!";
                    return ret;
                }
                state = EndBoundaryLF;
                break;
            case EndBoundaryLF:
                if (buffer[i] != '\n') {
                    qCDebug(CUTELYST_MULTIPART) << "EndBoundaryLF return!";
                    return ret;
                }
                header.clear();
                state = StartHeaders;
                break;
            case StartHeaders:
                qCDebug(CUTELYST_MULTIPART) << "StartHeaders" << body->pos() - len + i;
                if (buffer[i] == '\r') {
                    state = EndHeaders;
                } else if (buffer[i] == '-') {
                    qCDebug(CUTELYST_MULTIPART) << "StartHeaders return!";
                    return ret;
                } else {
                    char *pch = strchr(buffer + i, '\r');
                    if (pch == NULL) {
                        header.append(buffer + i, len - i);
                        i = len;
                    } else {
                        header.append(buffer + i, pch - buffer - i);
                        i = pch - buffer;
                        state = FinishHeader;
                    }
                }
                break;
            case FinishHeader:
                qCDebug(CUTELYST_MULTIPART) << "FinishHeader" << header;
                if (buffer[i] == '\n') {
                    int dotdot = header.indexOf(':');
                    headers.setHeader(header.left(dotdot), header.mid(dotdot + 1).trimmed());
                    header.clear();
                    state = StartHeaders;
                } else {
                    qCDebug(CUTELYST_MULTIPART) << "FinishHeader return!";
                    return ret;
                }
                break;
            case EndHeaders:
                qCDebug(CUTELYST_MULTIPART) << "EndHeaders";
                if (buffer[i] == '\n') {
                    state = StartData;
                } else {
                    qCDebug(CUTELYST_MULTIPART) << "EndHeaders return!";
                    return ret;
                }
                break;
            case StartData:
                qCDebug(CUTELYST_MULTIPART) << "StartData" << body->pos() - len + i;
                startOffset = body->pos() - len + i;
                state = EndData;
            case EndData:
                i += findBoundary(buffer + i, len - i, state, boundaryPos);
                if (state == EndBoundaryCR) {
                    qCDebug(CUTELYST_MULTIPART) << "EndData" << body->pos() - len + i - boundaryLength - 1;
                    UploadPrivate *priv = new UploadPrivate(body);
                    priv->headers = headers;
                    headers.clear();
                    priv->startOffset = startOffset;
                    priv->endOffset = body->pos() - len + i - boundaryLength - 1;
                    ret << new Upload(priv);
                }
            }
            ++i;
        }
    }

    return ret;
}

int MultiPartFormDataParserPrivate::findBoundary(char *buffer, int len, MultiPartFormDataParserPrivate::ParserState &state, int &boundaryPos)
{
    int i = 0;
    while (i < len) {
        if (buffer[i] == boundary[boundaryPos]) {
            if (++boundaryPos == boundaryLength) {
                qCDebug(CUTELYST_MULTIPART) << "FindBoundary:" << body->pos() - len + i;
                boundaryPos = 0;
                state = EndBoundaryCR;
                return i;
            }
        } else {
            boundaryPos = 0;
        }
        ++i;
    }
    return i;
}
