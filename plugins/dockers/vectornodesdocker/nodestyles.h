#ifndef NODESTYLES_H
#define NODESTYLES_H

#include <QColor>

namespace NodeStyles {

class Style {
public:
    // Background
    static int GridSize;
    // Nodes
    static int NodeHeaderHeight;
    static int NodeMarginV;
};

class Color {
public:
    // Background
    static QColor BackgroundFill;
    static QColor GridLinesStroke;
    // Nodes
    //    Empty
    //    Value
    //    Color
    //    Geometry
    //    Attribute
    //    Procedural TODO
    static QColor NodeFill_Normie;
    static QColor HeaderGeometry_Normie;
    static QColor HeaderColor_Normie;
    static QColor HeaderValue_Normie;
    static QColor HeaderIO_Normie;
    static QColor HeaderAdjust_Normie;
    static QColor Outline_Normie;
    static QColor Text_Normie;
    static QColor IO_Normie;
    static QColor IO_Pen_Normie;
    static QColor Pen_Hover_Normie;
    static QColor V2_Normie;
    static QColor Connection_Normie;
    static QColor Connection_Selected_Normie;
};

}

#endif // NODESTYLES_H
