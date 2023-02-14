#include "nodestyles.h"

namespace NodeStyles {
// Styles
    // Background
    int Style::GridSize = 20;
    // Nodes
    int Style::NodeHeaderHeight = 18;
    int Style::NodeMarginV = 14;

// Color
    // Background
    QColor Color::BackgroundFill = QColor(80, 80, 80, 255);
    QColor Color::GridLinesStroke = QColor(130, 130, 130, 255);
    // Nodes
    QColor Color::NodeFill_Normie = QColor(48, 48, 48, 255);
    QColor Color::HeaderGeometry_Normie = QColor(29, 114, 94, 255);
    QColor Color::HeaderColor_Normie = QColor(225, 114, 128, 255);
    QColor Color::HeaderValue_Normie = QColor(131, 49, 74, 255);
    QColor Color::HeaderIO_Normie = QColor(29, 29, 29, 255);
    QColor Color::HeaderAdjust_Normie = QColor(29, 37, 70, 255);
    QColor Color::Outline_Normie = QColor(20, 20, 20, 255);
    QColor Color::Text_Normie = QColor(229, 229, 229, 255);
    QColor Color::IO_Normie = QColor(161, 161, 161, 255);
    QColor Color::IO_Pen_Normie = QColor(29, 29, 29, 255);
    QColor Color::Pen_Hover_Normie = QColor(255, 231, 216, 255);
    QColor Color::V2_Normie = QColor(99, 99, 199, 255);
    QColor Color::Connection_Normie = QColor(161, 161, 161, 255);
    QColor Color::Connection_Selected_Normie = QColor(255, 99, 199, 255);
}
