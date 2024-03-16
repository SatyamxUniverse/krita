/*
 *  SPDX-FileCopyrightText: 2024 Grum999 <grum999@grum.fr>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 *
 */

#include "kis_qoi_converter.h"

#include <QColorSpace>
#include <QFile>
#include <QIODevice>
#include <QImage>
#include <QImageIOHandler>

// documentation of Quite OK Image (QOI) file format available at:
//      https://qoiformat.org/qoi-specification.pdf
// original QOI encoder/decoder:
//      https://github.com/phoboslab/qoi/tree/master
// original QOI website:
//      https://qoiformat.org
//
// this implementation is mostly based from KDE's KImageFormats implementation
//      https://invent.kde.org/frameworks/kimageformats/-/blob/master/src/imageformats/qoi.cpp

namespace
{

#define QOI_OP_INDEX 0x00   /* 00xxxxxx */
#define QOI_OP_DIFF 0x40    /* 01xxxxxx */
#define QOI_OP_LUMA 0x80    /* 10xxxxxx */
#define QOI_OP_RUN 0xc0     /* 11xxxxxx */
#define QOI_OP_RGB 0xfe     /* 11111110 */
#define QOI_OP_RGBA 0xff    /* 11111111 */
#define QOI_MASK_2 0xc0     /* 11000000 */

#define QOI_MAGIC (((unsigned int)'q') << 24 | ((unsigned int)'o') << 16 | ((unsigned int)'i') << 8 | ((unsigned int)'f'))
#define QOI_HEADER_SIZE 14
#define QOI_END_STREAM_PAD 8

// QList uses some extra space for stuff, hence the 32 here suggested by Thiago Macieira
static constexpr int kMaxQVectorSize = std::numeric_limits<int>::max() - 32;

// On Qt 6 to make the plugins fail to allocate if the image size is greater than QImageReader::allocationLimit()
// it is necessary to allocate the image with QImageIOHandler::allocateImage().
inline QImage imageAlloc(const QSize &size, const QImage::Format &format)
{
    QImage img = QImage(size, format);
    return img;
}

inline QImage imageAlloc(qint32 width, qint32 height, const QImage::Format &format)
{
    return imageAlloc(QSize(width, height), format);
}



struct QoiHeader {
    quint32 MagicNumber;
    quint32 Width;
    quint32 Height;
    quint8 Channels;
    quint8 Colorspace;
};

struct Px {
    bool operator==(const Px &other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    quint8 r;
    quint8 g;
    quint8 b;
    quint8 a;
};

static QDataStream &operator>>(QDataStream &s, QoiHeader &head)
{
    s >> head.MagicNumber;
    s >> head.Width;
    s >> head.Height;
    s >> head.Channels;
    s >> head.Colorspace;
    return s;
}

static QDataStream &operator<<(QDataStream &s, const QoiHeader &head)
{
    s << head.MagicNumber;
    s << head.Width;
    s << head.Height;
    s << head.Channels;
    s << head.Colorspace;
    return s;
}

static bool isSupported(const QoiHeader &head)
{
    // Check magic number
    if (head.MagicNumber != QOI_MAGIC) {
        return false;
    }
    // Check if the header is a valid QOI header
    if (head.Width == 0 || head.Height == 0 || head.Channels < 3 || head.Colorspace > 1) {
        return false;
    }
    // Set a reasonable upper limit
    if (head.Width > 300000 || head.Height > 300000) {
        return false;
    }
    return true;
}

static int qoiHash(const Px &px)
{
    return px.r * 3 + px.g * 5 + px.b * 7 + px.a * 11;
}

static QImage::Format imageFormat(const QoiHeader &head)
{
    if (isSupported(head)) {
        return (head.Channels == 3 ? QImage::Format_RGB32 : QImage::Format_ARGB32);
    }
    return QImage::Format_Invalid;
}

static KisImportExportErrorCode loadQOI(QIODevice *device, const QoiHeader &qoi, QImage &img)
{
    Px index[64] = {Px{0, 0, 0, 0}};
    Px px = Px{0, 0, 0, 255};

    // The px_len should be enough to read a complete "compressed" row: an uncompressible row can become
    // larger than the row itself. It should never be more than 1/3 (RGB) or 1/4 (RGBA) the length of the
    // row itself (see test bnm_rgb*.qoi) so I set the extra data to 1/2.
    // The minimum value is to ensure that enough bytes are read when the image is very small (e.g. 1x1px):
    // it can be set as large as you like.
    quint64 px_len = std::max(quint64(1024), quint64(qoi.Width) * qoi.Channels * 3 / 2);
    if (px_len > kMaxQVectorSize) {
        return ImportExportCodes::FileFormatIncorrect;
    }

    // Allocate image
    img = QImage(QSize(qoi.Width, qoi.Height), imageFormat(qoi));
    if (img.isNull()) {
        return ImportExportCodes::Failure;
    }

    // Set the image colorspace based on the qoi.Colorspace value
    // As per specification: 0 = sRGB with linear alpha, 1 = all channels linear
    if (qoi.Colorspace) {
        img.setColorSpace(QColorSpace(QColorSpace::SRgbLinear));
    } else {
        img.setColorSpace(QColorSpace(QColorSpace::SRgb));
    }

    // Handle the byte stream
    QByteArray ba;
    for (quint32 y = 0, run = 0; y < qoi.Height; ++y) {
        if (quint64(ba.size()) < px_len) {
            ba.append(device->read(px_len));
        }

        if (ba.size() < QOI_END_STREAM_PAD) {
            return ImportExportCodes::FileFormatIncorrect;
        }

        quint64 chunks_len = ba.size() - QOI_END_STREAM_PAD;
        quint64 p = 0;
        QRgb *scanline = reinterpret_cast<QRgb *>(img.scanLine(y));
        const quint8 *input = reinterpret_cast<const quint8 *>(ba.constData());
        for (quint32 x = 0; x < qoi.Width; ++x) {
            if (run > 0) {
                run--;
            } else if (p < chunks_len) {
                quint32 b1 = input[p++];

                if (b1 == QOI_OP_RGB) {
                    px.r = input[p++];
                    px.g = input[p++];
                    px.b = input[p++];
                } else if (b1 == QOI_OP_RGBA) {
                    px.r = input[p++];
                    px.g = input[p++];
                    px.b = input[p++];
                    px.a = input[p++];
                } else if ((b1 & QOI_MASK_2) == QOI_OP_INDEX) {
                    px = index[b1];
                } else if ((b1 & QOI_MASK_2) == QOI_OP_DIFF) {
                    px.r += ((b1 >> 4) & 0x03) - 2;
                    px.g += ((b1 >> 2) & 0x03) - 2;
                    px.b += (b1 & 0x03) - 2;
                } else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA) {
                    quint32 b2 = input[p++];
                    quint32 vg = (b1 & 0x3f) - 32;
                    px.r += vg - 8 + ((b2 >> 4) & 0x0f);
                    px.g += vg;
                    px.b += vg - 8 + (b2 & 0x0f);
                } else if ((b1 & QOI_MASK_2) == QOI_OP_RUN) {
                    run = (b1 & 0x3f);
                }
                index[qoiHash(px) & 0x3F] = px;
            }
            // Set the values for the pixel at (x, y)
            scanline[x] = qRgba(px.r, px.g, px.b, px.a);
        }

        if (p) {
            ba.remove(0, p);
        }
    }

    // From specs the byte stream's end is marked with 7 0x00 bytes followed by a single 0x01 byte.
    // NOTE: Instead of using "ba == QByteArray::fromRawData("\x00\x00\x00\x00\x00\x00\x00\x01", 8)"
    //       we preferred a generic check that allows data to exist after the end of the file.
    if (ba.startsWith(QByteArray::fromRawData("\x00\x00\x00\x00\x00\x00\x00\x01", 8))) {
        return ImportExportCodes::OK;
    }
    return ImportExportCodes::FormatFeaturesUnsupported;
}

static KisImportExportErrorCode saveQOI(QIODevice *device, const QoiHeader &qoi, const QImage &img)
{
    Px index[64] = {Px{0, 0, 0, 0}};
    Px px = Px{0, 0, 0, 255};
    Px px_prev = px;

    quint32 run = 0;
    quint8 channels = qoi.Channels;

    QByteArray ba;
    ba.reserve(img.width() * channels * 3 / 2);

    for (quint32 h = img.height(), y = 0; y < h; ++y) {

        const uchar *pixels = img.constScanLine(y);
        if (pixels == nullptr) {
            return ImportExportCodes::ErrorWhileWriting;
        }

        for (quint32 w = img.width() * channels, px_pos = 0; px_pos < w; px_pos += channels) {
            px.r = pixels[px_pos + 0];
            px.g = pixels[px_pos + 1];
            px.b = pixels[px_pos + 2];

            if (channels == 4) {
                px.a = pixels[px_pos + 3];
            }

            if (px == px_prev) {
                run++;
                if (run == 62 || (px_pos == w - channels && y == h - 1)) {
                    ba.append(QOI_OP_RUN | (run - 1));
                    run = 0;
                }
            } else {
                int index_pos;

                if (run > 0) {
                    ba.append(QOI_OP_RUN | (run - 1));
                    run = 0;
                }

                index_pos = qoiHash(px) & 0x3F;

                if (index[index_pos] == px) {
                    ba.append(QOI_OP_INDEX | index_pos);
                } else {
                    index[index_pos] = px;

                    if (px.a == px_prev.a) {
                        signed char vr = px.r - px_prev.r;
                        signed char vg = px.g - px_prev.g;
                        signed char vb = px.b - px_prev.b;

                        signed char vg_r = vr - vg;
                        signed char vg_b = vb - vg;

                        if (vr > -3 && vr < 2 && vg > -3 && vg < 2 && vb > -3 && vb < 2) {
                            ba.append(QOI_OP_DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2));
                        } else if (vg_r > -9 && vg_r < 8 && vg > -33 && vg < 32 && vg_b > -9 && vg_b < 8) {
                            ba.append(QOI_OP_LUMA | (vg + 32));
                            ba.append((vg_r + 8) << 4 | (vg_b + 8));
                        } else {
                            ba.append(char(QOI_OP_RGB));
                            ba.append(px.r);
                            ba.append(px.g);
                            ba.append(px.b);
                        }
                    } else {
                        ba.append(char(QOI_OP_RGBA));
                        ba.append(px.r);
                        ba.append(px.g);
                        ba.append(px.b);
                        ba.append(px.a);
                    }
                }
            }
            px_prev = px;
        }

        qint64 written = device->write(ba);
        if (written < 0) {
            return ImportExportCodes::ErrorWhileWriting;
        }
        if (written) {
            ba.remove(0, written);
        }
    }

    // QOI end of stream
    ba.append(QByteArray::fromRawData("\x00\x00\x00\x00\x00\x00\x00\x01", 8));

    // write remaining data
    for (qint64 w = 0, write = 0, size = ba.size(); write < size; write += w) {
        w = device->write(ba.constData() + write, size - write);
        if (w < 0) {
            return ImportExportCodes::ErrorWhileWriting;
        }
    }

    return ImportExportCodes::OK;
}

} // namespace



struct KisQOIConverter::Private
{
    Private(KisDocument *doc, bool batchMode)
        : doc(doc),
          batchMode(batchMode)
    {}

    QImage image;
    KisDocument *doc;
    bool batchMode;
};

KisQOIConverter::KisQOIConverter(KisDocument *doc, bool batchMode)
    : m_d(new Private(doc, batchMode))
{
}

KisQOIConverter::~KisQOIConverter()
{
}

KisImportExportErrorCode KisQOIConverter::read(QIODevice *io)
{
    QDataStream stream(io);
    stream.setByteOrder(QDataStream::BigEndian);

    // Read image header
    QoiHeader qoi = {0, 0, 0, 0, 2};
    stream >> qoi;

    // Check if file is supported
    if (!isSupported(qoi)) {
        return ImportExportCodes::FileFormatIncorrect;
    }

    // load image
    QImage img;
    KisImportExportErrorCode result = loadQOI(stream.device(), qoi, img);
    if (result == ImportExportCodes::OK || result == ImportExportCodes::FormatFeaturesUnsupported) {
        m_d->image = img;
    }

    return result;
}

KisImportExportErrorCode KisQOIConverter::write(QIODevice *io, const QImage &image)
{
    if (image.isNull()) {
        return ImportExportCodes::Failure;
    }

    // generate header
    QoiHeader qoi;
    qoi.MagicNumber = QOI_MAGIC;
    qoi.Width = image.width();
    qoi.Height = image.height();
    qoi.Channels = image.hasAlphaChannel() ? 4 : 3;
    qoi.Colorspace = image.colorSpace().transferFunction() == QColorSpace::TransferFunction::Linear ? 1 : 0;

    if (!isSupported(qoi)) {
        return ImportExportCodes::FileFormatNotSupported;
    }

    QDataStream stream(io);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << qoi;
    if (stream.status() != QDataStream::Ok) {
        return ImportExportCodes::ErrorWhileWriting;
    }

    return saveQOI(stream.device(), qoi, image);
}

QImage KisQOIConverter::image()
{
    return m_d->image;
}

bool KisQOIConverter::isColorSpaceSupported(const KoColorSpace *cs)
{
    return cs->id() == "RGBA";
}


