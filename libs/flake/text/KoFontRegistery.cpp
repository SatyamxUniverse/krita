/*
 *  SPDX-FileCopyrightText: 2022 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KoFontRegistery.h"
#include "FlakeDebug.h"
#include "KoCssTextUtils.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGlobalStatic>
#include <QMutex>
#include <QThread>
#include <QThreadStorage>
#include <QtGlobal>
#include <utility>

#include <KoResourcePaths.h>
#include <kis_debug.h>

#include "KoFontLibraryResourceUtils.h"

Q_GLOBAL_STATIC(KoFontRegistery, s_instance)

class KoFontRegistery::Private
{
private:
    struct ThreadData {
        FcConfigUP m_config;
        FT_LibraryUP m_library;
        QHash<FcChar32, FcPatternUP> m_patterns;
        QHash<FcChar32, FcFontSetUP> m_fontSets;
        QHash<QString, FT_FaceUP> m_faces;

        ThreadData(FcConfigUP cfg, FT_LibraryUP lib)
            : m_config(std::move(cfg))
            , m_library(std::move(lib))
        {
        }
    };

    QThreadStorage<QSharedPointer<ThreadData>> m_data;

    void initialize()
    {
        if (!m_data.hasLocalData()) {
            FT_Library lib = nullptr;
            FcConfig *config = FcConfigCreate();
            KIS_ASSERT(config && "No Fontconfig support available");
            if (qgetenv("FONTCONFIG_PATH").isEmpty()) {
                QDir appdir(KoResourcePaths::getApplicationRoot()
                            + "/etc/fonts");
                if (QFile::exists(appdir.absoluteFilePath("fonts.conf"))) {
                    qputenv("FONTCONFIG_PATH",
                            QFile::encodeName(QDir::toNativeSeparators(
                                appdir.absolutePath())));
                }
            }
            debugFlake << "Setting FONTCONFIG_PATH"
                       << qgetenv("FONTCONFIG_PATH");
            if (!FcConfigParseAndLoad(config, nullptr, FcTrue)) {
                errorFlake << "Failed loading the Fontconfig configuration";
            } else {
                FcConfigSetCurrent(config);
            }
            FT_Error error = FT_Init_FreeType(&lib);
            if (error) {
                errorFlake << "Error with initializing FreeType library:"
                           << error
                           << "Current thread:" << QThread::currentThread()
                           << "GUI thread:" << qApp->thread();
            } else {
                m_data.setLocalData(
                    QSharedPointer<ThreadData>::create(config, lib));
            }
        }
    }

public:
    FT_LibraryUP library()
    {
        if (!m_data.hasLocalData())
            initialize();
        return m_data.localData()->m_library;
    }

    ~Private() = default;

    QHash<FcChar32, FcPatternUP> &patterns()
    {
        if (!m_data.hasLocalData())
            initialize();
        return m_data.localData()->m_patterns;
    }

    QHash<FcChar32, FcFontSetUP> &sets()
    {
        if (!m_data.hasLocalData())
            initialize();
        return m_data.localData()->m_fontSets;
    }

    QHash<QString, FT_FaceUP> &typeFaces()
    {
        if (!m_data.hasLocalData())
            initialize();
        return m_data.localData()->m_faces;
    }
};

KoFontRegistery::KoFontRegistery()
    : d(new Private())
{
}

KoFontRegistery *KoFontRegistery::instance()
{
    return s_instance;
}

std::vector<FT_FaceUP> KoFontRegistery::facesForCSSValues(QStringList families,
                                                          QVector<int> &lengths,
                                                          QString text,
                                                          qreal size,
                                                          int weight,
                                                          int width,
                                                          bool italic,
                                                          int slant,
                                                          QString language)
{
    Q_UNUSED(size)
    Q_UNUSED(language)
    // FcObjectSet *objectSet = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_WIDTH,
    // FC_WEIGHT, FC_SLANT, nullptr);
    FcPatternUP p(FcPatternCreate());
    for (const QString &family : families) {
        QByteArray utfData = family.toUtf8();
        const FcChar8 *vals = reinterpret_cast<FcChar8 *>(utfData.data());
        FcPatternAddString(p.data(), FC_FAMILY, vals);
    }
    if (italic || slant != 0) {
        FcPatternAddInteger(p.data(), FC_SLANT, FC_SLANT_ITALIC);
    } else {
        FcPatternAddInteger(p.data(), FC_SLANT, FC_SLANT_ROMAN);
    }
    FcPatternAddInteger(p.data(), FC_WEIGHT, FcWeightFromOpenType(weight));
    FcPatternAddInteger(p.data(), FC_WIDTH, width);
    FcPatternAddBool(p.data(), FC_OUTLINE, true);

    p = [&]() {
        const FcChar32 hash = FcPatternHash(p.data());
        auto oldPattern = d->patterns().find(hash);
        if (oldPattern != d->patterns().end()) {
            return oldPattern.value();
        } else {
            d->patterns().insert(hash, p);
            return p;
        }
    }();

    FcResult result = FcResultNoMatch;
    FcChar8 *fileValue = nullptr;
    FcCharSetUP charSet;
    FcFontSetUP fontSet = [&]() -> FcFontSetUP {
        const FcChar32 hash = FcPatternHash(p.data());
        auto set = d->sets().find(hash);

        if (set != d->sets().end()) {
            return set.value();
        } else {
            FcCharSet *cs = nullptr;
            KisLibraryResourcePointer<FcFontSet, FcFontSetDestroy> avalue(
                FcFontSort(FcConfigGetCurrent(),
                           p.data(),
                           FcTrue,
                           &cs,
                           &result));
            charSet.reset(cs);
            d->sets().insert(hash, avalue);
            return avalue;
        }
    }();

    QStringList fontFileNames;
    lengths.clear();

    if (text.isEmpty()) {
        for (int j = 0; j < fontSet->nfont; j++) {
            QString fontFileName;
            if (FcPatternGetString(fontSet->fonts[j], FC_FILE, 0, &fileValue)
                == FcResultMatch) {
                fontFileName = QString(reinterpret_cast<char *>(fileValue));
                fontFileNames.append(fontFileName);
                lengths.append(0);
                break;
            }
        }
    } else {
        QString fontFileName;
        FcChar8 *fileValue = 0;
        FcCharSet *set = 0;
        QVector<int> familyValues(text.size());
        QVector<int> fallbackMatchValues(text.size());
        familyValues.fill(-1);
        fallbackMatchValues.fill(-1);


        // First, we're going to split up the text into graphemes. This is both
        // because the css spec requires it, but also because of why the css
        // spec requires it: graphemes' parts should not end up in seperate
        // runs, which they will if they get assigned different fonts,
        // potentially breaking ligatures and emoji sequences.
        QStringList graphemes =
            KoCssTextUtils::textToUnicodeGraphemeClusters(text, language);

        // Parse over the fonts and graphemes and try to see if we can get the
        // best match for a given grapheme.
        for (int i = 0; i < fontSet->nfont; i++) {
            if (FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &set)
                == FcResultMatch) {
                int index = 0;
                for (QString grapheme : graphemes) {
                    int familyIndex = -1;
                    if (familyValues.at(index) == -1) {
                        int fallbackMatch = fallbackMatchValues.at(index);
                        for (uint unicode : grapheme.toUcs4()) {
                            if (FcCharSetHasChar(set, unicode)) {
                                familyIndex = i;
                                if (fallbackMatch < 0) {
                                    fallbackMatch = i;
                                }
                            } else {
                                familyIndex = -1;
                                break;
                            }
                        }
                        for (int k = 0; k < grapheme.size(); k++) {
                            familyValues[index + k] = familyIndex;
                            fallbackMatchValues[index + k] = fallbackMatch;
                        }
                    }
                    index += grapheme.size();
                }
                if (!familyValues.contains(-1)) {
                    break;
                }
            }
        }

        // Remove the -1 entries.
        if (familyValues.contains(-1)) {
            int val = -1;
            for (int i : familyValues) {
                if (i != val) {
                    val = i;
                    break;
                }
            }
            val = qMax(0, val);
            for (int i = 0; i < familyValues.size(); i++) {
                if (familyValues.at(i) < 0) {
                    if (fallbackMatchValues.at(i) < 0) {
                        familyValues[i] = val;
                    } else {
                        familyValues[i] = fallbackMatchValues.at(i);
                    }
                } else {
                    val = familyValues.at(i);
                }
            }
        }

        // Get the filenames and lengths for the entries.
        int length = 0;
        int startIndex = 0;
        int lastIndex = familyValues.at(0);
        if (FcPatternGetString(fontSet->fonts[lastIndex],
                               FC_FILE,
                               0,
                               &fileValue)
            == FcResultMatch) {
            fontFileName = QString(reinterpret_cast<char *>(fileValue));
        }
        for (int i = 0; i < familyValues.size(); i++) {
            if (lastIndex != familyValues.at(i)) {
                lengths.append(text.mid(startIndex, length).size());
                fontFileNames.append(fontFileName);
                startIndex = i;
                length = 0;
                lastIndex = familyValues.at(i);
                if (FcPatternGetString(fontSet->fonts[lastIndex],
                                       FC_FILE,
                                       0,
                                       &fileValue)
                    == FcResultMatch) {
                    fontFileName = QString(reinterpret_cast<char *>(fileValue));
                }
            }
            length += 1;
        }
        if (length > 0) {
            lengths.append(text.mid(startIndex, length).size());
            fontFileNames.append(fontFileName);
        }
    }

    std::vector<FT_FaceUP> faces;

    for (int i = 0; i < lengths.size(); i++) {
        auto entry = d->typeFaces().find(fontFileNames[i]);
        if (entry != d->typeFaces().end()) {
            faces.emplace_back(entry.value());
        } else {
            FT_Face f = nullptr;
            QByteArray utfData = fontFileNames.at(i).toUtf8();
            if (FT_New_Face(d->library().data(), utfData.data(), 0, &f) == 0) {
                FT_FaceUP face(f);
                faces.emplace_back(face);
                d->typeFaces().insert(fontFileNames[i], face);
            }
        }
    }

    return faces;
}

bool KoFontRegistery::configureFaces(const std::vector<FT_FaceUP> &faces,
                                     qreal size,
                                     qreal fontSizeAdjust,
                                     int xRes,
                                     int yRes,
                                     QMap<QString, qreal> axisSettings)
{
    int errorCode = 0;
    int ftFontUnit = 64.0;
    qreal finalRes = qMin(xRes, yRes);
    qreal scaleToPixel = float(finalRes / 72.);
    for (const FT_FaceUP &face : faces) {
        if (!FT_IS_SCALABLE(face)) {
            int fontSizePixels = size * ftFontUnit * scaleToPixel;
            int sizeDelta = 0;
            int selectedIndex = -1;

            for (int i = 0; i < face->num_fixed_sizes; i++) {
                int newDelta =
                    qAbs((fontSizePixels)-face->available_sizes[i].x_ppem);
                if (newDelta < sizeDelta || i == 0) {
                    selectedIndex = i;
                    sizeDelta = newDelta;
                }
            }

            if (selectedIndex >= 0) {
                if (FT_HAS_COLOR(face)) {
                    long scale = long(
                        65535 * qreal(fontSizePixels)
                        / qreal(face->available_sizes[selectedIndex].x_ppem));
                    FT_Matrix matrix;
                    matrix.xx = scale;
                    matrix.xy = 0;
                    matrix.yx = 0;
                    matrix.yy = scale;
                    FT_Vector v;
                    FT_Set_Transform(face.data(), &matrix, &v);
                }
                errorCode = FT_Select_Size(face.data(), selectedIndex);
            }
        } else {
            errorCode =
                FT_Set_Char_Size(face.data(), size * ftFontUnit, 0, xRes, yRes);
            hb_font_t_up font(hb_ft_font_create_referenced(face.data()));
            hb_position_t xHeight = 0;
            hb_ot_metrics_get_position(font.data(),
                                       HB_OT_METRICS_TAG_X_HEIGHT,
                                       &xHeight);
            if (xHeight > 0 && fontSizeAdjust > 0 && fontSizeAdjust < 1.0) {
                qreal aspect = xHeight / (size * ftFontUnit * scaleToPixel);
                errorCode = FT_Set_Char_Size(face.data(),
                                             (fontSizeAdjust / aspect)
                                                 * (size * ftFontUnit),
                                             0,
                                             xRes,
                                             yRes);
            }
        }

        QMap<FT_Tag, qreal> tags;
        for (QString tagName : axisSettings.keys()) {
            if (tagName.size() == 4) {
                QByteArray utfData = tagName.toUtf8();
                char *t = utfData.data();
                tags.insert(FT_MAKE_TAG(t[0], t[1], t[2], t[3]),
                            axisSettings.value(tagName));
            }
        }
        if (FT_HAS_MULTIPLE_MASTERS(face)) {
            FT_MM_Var *amaster = nullptr;
            FT_Get_MM_Var(face.data(), &amaster);
            // note: this only works for opentype, as it uses
            // tag-based-selection.
            std::vector<FT_Fixed> designCoords(amaster->num_axis);
            for (FT_UInt i = 0; i < amaster->num_axis; i++) {
                FT_Var_Axis axis = amaster->axis[i];
                designCoords[i] = axis.def;
                for (FT_Tag tag : tags.keys()) {
                    if (axis.tag == tag) {
                        designCoords[i] = qBound(axis.minimum,
                                                 long(tags.value(tag) * 65535),
                                                 axis.maximum);
                    }
                }
            }
            FT_Set_Var_Design_Coordinates(face.data(),
                                          amaster->num_axis,
                                          designCoords.data());
            FT_Done_MM_Var(d->library().data(), amaster);
        }
    }
    return (errorCode == 0);
}

bool KoFontRegistery::addFontFilePathToRegistery(QString path)
{
    QByteArray utfData = path.toUtf8();
    const FcChar8 *vals = reinterpret_cast<FcChar8 *>(utfData.data());
    return FcConfigAppFontAddFile(FcConfigGetCurrent(), vals);
}

bool KoFontRegistery::addFontFileDirectoryToRegistery(QString path)
{
    QByteArray utfData = path.toUtf8();
    const FcChar8 *vals = reinterpret_cast<FcChar8 *>(utfData.data());
    return FcConfigAppFontAddDir(FcConfigGetCurrent(), vals);
}
