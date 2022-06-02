/*
 *  SPDX-FileCopyrightText: 2022 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KoFontRegistery.h"
#include "KoCssTextUtils.h"

#include <QGlobalStatic>
#include <QMutex>
#include <QDebug>
#include <QThreadStorage>
#include <QThread>
#include <QApplication>

#include "KoFontLibraryResourceUtils.h"

Q_GLOBAL_STATIC(KoFontRegistery, s_instance)

class KoFontRegistery::Private
{
private:
    QThreadStorage<FT_LibraryUP*> m_library;

public:
    FT_LibraryUP& library() {
        if (!m_library.hasLocalData()) {
            FT_Error error;
            FT_LibraryUP *lib = new FT_LibraryUP();
            error = FT_Init_FreeType(lib->externalInitialization());
            if (error) {
                qWarning() << "Error with initializing FreeType library:" << error
                           << "Current thread:" << QThread::currentThread()
                           << "GUI thread:" << qApp->thread();
            } else {
                m_library.setLocalData(lib);
            }
        }
        return *m_library.localData();
    }

};

KoFontRegistery::KoFontRegistery() : d(new Private())
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
    //FcObjectSet *objectSet = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_WIDTH, FC_WEIGHT, FC_SLANT, nullptr);
    FcPatternUP p = toLibraryResource(FcPatternCreate());
    for (QString family: families) {
        QByteArray utfData = family.toUtf8();
        const FcChar8 *vals = reinterpret_cast<FcChar8*>(utfData.data());
        FcPatternAddString(p.data(), FC_FAMILY, vals);
    }
    if (italic == true) {
        FcPatternAddInteger(p.data(), FC_SLANT, FC_SLANT_ITALIC);
    } else if(slant != 0) {
        FcPatternAddInteger(p.data(), FC_SLANT, FC_SLANT_ITALIC);
    } else {
        FcPatternAddInteger(p.data(), FC_SLANT, FC_SLANT_ROMAN);
    }
    FcPatternAddInteger(p.data(), FC_WEIGHT, FcWeightFromOpenType(weight));
    FcPatternAddInteger(p.data(), FC_WIDTH, width);
    FcPatternAddBool(p.data(), FC_OUTLINE, true);

    FcResult result;
    FcChar8 *fileValue = 0;
    FcCharSetUP charSet;
    FcFontSetUP fontSet = toLibraryResource(FcFontSort(FcConfigGetCurrent(), p.data(), FcTrue, charSet.externalInitialization(), &result));

    QStringList fontFileNames;
    lengths.clear();

    if (text.isEmpty()) {
        for (int j = 0; j < fontSet->nfont; j++) {
            QString fontFileName;
            if (FcPatternGetString(fontSet->fonts[j], FC_FILE, 0, &fileValue) == FcResultMatch) {
                fontFileName = QString(reinterpret_cast<char*>(fileValue));
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
        familyValues.fill(-1);

        // First, we're going to split up the text into graphemes. This is both because the css
        // spec requires it, but also because of why the css spec requires it: graphemes' parts should
        // not end up in seperate runs, which they will if they get assigned different fonts, potentially breaking
        // ligatures and emoji sequences.
        QStringList graphemes = KoCssTextUtils::textToUnicodeGraphemeClusters(text, language);

        // Parse over the fonts and graphemes and try to see if we can get the best match for a given grapheme.
        for (int i=0; i<fontSet->nfont; i++) {
            if (FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &set) == FcResultMatch) {
                int index = 0;
                for (QString grapheme : graphemes) {
                    int familyIndex = -1;
                    if (familyValues.at(index) == -1) {
                        for (uint unicode : grapheme.toUcs4()) {

                            if (FcCharSetHasChar(set, unicode)) {
                                familyIndex = i;
                            } else {
                                familyIndex = -1;
                                break;
                            }
                        }
                        for (int k = 0; k < grapheme.size(); k++) {
                            familyValues[index+k] = familyIndex;
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
            for (int i: familyValues) {
                if (i != val) {
                    val = i;
                    break;
                }
            }
            val = qMax(0, val);
            for (int i = 0; i < familyValues.size(); i++) {
                if (familyValues.at(i) < 0) {
                    familyValues[i] = val;
                } else {
                    val = familyValues.at(i);
                }
            }
        }

        // Get the filenames and lengths for the entries.
        int length = 0;
        int startIndex = 0;
        int lastIndex = familyValues.at(0);
        if (FcPatternGetString(fontSet->fonts[lastIndex], FC_FILE, 0, &fileValue) == FcResultMatch) {
            fontFileName = QString(reinterpret_cast<char*>(fileValue));
        }
        for (int i = 0; i< familyValues.size(); i++) {
            if (lastIndex != familyValues.at(i)) {
                lengths.append(text.mid(startIndex, length).size());
                fontFileNames.append(fontFileName);
                startIndex = i;
                length = 0;
                lastIndex = familyValues.at(i);
                if (FcPatternGetString(fontSet->fonts[lastIndex], FC_FILE, 0, &fileValue) == FcResultMatch) {
                    fontFileName = QString(reinterpret_cast<char*>(fileValue));
                }
            }
            length +=1;
        }
        if (length > 0) {
            lengths.append(text.mid(startIndex, length).size());
            fontFileNames.append(fontFileName);
        }
    }

    std::vector<FT_FaceUP> faces;

    for (int i = 0; i < lengths.size(); i++) {
        FT_FaceUP face;
        QByteArray utfData = fontFileNames.at(i).toUtf8();
        if (FT_New_Face(d->library().data(), utfData.data(), 0, face.externalInitialization()) == 0) {
            faces.emplace_back(std::move(face));
        }
    }

    return faces;
}

bool KoFontRegistery::configureFaces(std::vector<FT_FaceUP> &faces,
                                     qreal size, qreal fontSizeAdjust,
                                     int xRes,
                                     int yRes,
                                     QMap<QString, qreal> axisSettings)
{
    int errorCode = 0;
    int ftFontUnit = 64.0;
    qreal finalRes = qMin(xRes, yRes);
    qreal scaleToPixel = float(finalRes/72.);
    for (FT_FaceUP &face: faces) {
        if (!FT_IS_SCALABLE(face)) {
            int fontSizePixels = size * ftFontUnit * scaleToPixel;
            int sizeDelta = 0;
            int selectedIndex = -1;

            for (int i=0; i<face->num_fixed_sizes; i++) {
                int newDelta = qAbs((fontSizePixels) - face->available_sizes[i].x_ppem);
                if (newDelta < sizeDelta || i == 0) {
                    selectedIndex = i;
                    sizeDelta = newDelta;
                }
            }

            if (selectedIndex >= 0) {
                if (FT_HAS_COLOR(face)) {
                    long scale = long(65535 * qreal(fontSizePixels)/qreal(face->available_sizes[selectedIndex].x_ppem));
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
            errorCode = FT_Set_Char_Size(face.data(), size * ftFontUnit, 0, xRes, yRes);
            hb_font_t_up font = toLibraryResource(hb_ft_font_create_referenced(face.data()));
            hb_position_t xHeight = 0;
            hb_ot_metrics_get_position(font.data(), HB_OT_METRICS_TAG_X_HEIGHT, &xHeight);
            if (xHeight > 0 && fontSizeAdjust > 0 && fontSizeAdjust < 1.0) {
                qreal aspect = xHeight / (size * ftFontUnit * scaleToPixel);
                errorCode = FT_Set_Char_Size(face.data(), (fontSizeAdjust/aspect)*(size * ftFontUnit), 0, xRes, yRes);
            }

        }

        QMap<FT_Tag, qreal> tags;
        for (QString tagName: axisSettings.keys()) {
            if (tagName.size() == 4) {
                QByteArray utfData = tagName.toUtf8();
                char *t = utfData.data();
                tags.insert(FT_MAKE_TAG(t[0], t[1], t[2], t[3]), axisSettings.value(tagName));
            }
        }
        if (FT_HAS_MULTIPLE_MASTERS(face)) {
            FT_MM_Var*  amaster = nullptr;
            FT_Get_MM_Var(face.data(), &amaster);
            // note: this only works for opentype, as it uses tag-based-selection.
            std::vector<FT_Fixed> designCoords(amaster->num_axis);
            for (FT_UInt i = 0; i < amaster->num_axis; i++) {
                FT_Var_Axis axis = amaster->axis[i];
                designCoords[i] = axis.def;
                for (FT_Tag tag: tags.keys()) {
                    if (axis.tag == tag) {
                        designCoords[i] = qBound(axis.minimum,
                                                long(tags.value(tag) * 65535),
                                                axis.maximum);
                    }
                }
            }
            FT_Set_Var_Design_Coordinates(face.data(), amaster->num_axis, designCoords.data());
            FT_Done_MM_Var(d->library().data(), amaster);
        }
    }
    return (errorCode == 0);
}
