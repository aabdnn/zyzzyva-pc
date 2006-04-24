//---------------------------------------------------------------------------
// WordTableDelegate.cpp
//
// A class derived from QItemDelegate, used to render items in a word list.
//
// Copyright 2005, 2006 Michael W Thelen <mike@pietdepsi.com>.
//
// This file is part of Zyzzyva.
//
// Zyzzyva is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Zyzzyva is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//---------------------------------------------------------------------------

#include "WordTableDelegate.h"
#include "WordTableModel.h"
#include "Defs.h"
#include <QModelIndex>
#include <QPainter>

using namespace std;

const int WordTableDelegate::ITEM_XPADDING = 5;
const int WordTableDelegate::ITEM_YPADDING = 0;

const QColor VALID_NORMAL_WORD_FOREGROUND = Qt::black;
const QColor VALID_NORMAL_WORD_BACKGROUND = Qt::white;
const QColor VALID_NORMAL_WORD_OUTLINE = Qt::lightGray;
const QColor VALID_NORMAL_ALTERNATE_FOREGROUND = Qt::black;
const QColor VALID_NORMAL_ALTERNATE_BACKGROUND = Qt::lightGray;
const QColor VALID_NORMAL_ALTERNATE_OUTLINE = Qt::white;
const QColor VALID_CORRECT_WORD_FOREGROUND = Qt::black;
const QColor VALID_CORRECT_WORD_BACKGROUND = Qt::white;
const QColor VALID_CORRECT_WORD_OUTLINE = Qt::lightGray;
const QColor VALID_MISSED_WORD_FOREGROUND = Qt::black;
const QColor VALID_MISSED_WORD_BACKGROUND = Qt::cyan;
const QColor VALID_MISSED_WORD_OUTLINE = Qt::white;
const QColor LAST_ADDED_WORD_FOREGROUND = Qt::black;
const QColor LAST_ADDED_WORD_BACKGROUND = Qt::green;
const QColor LAST_ADDED_WORD_OUTLINE = Qt::white;
const QColor INVALID_WORD_FOREGROUND = Qt::black;
const QColor INVALID_WORD_BACKGROUND = Qt::red;
const QColor INVALID_WORD_OUTLINE = Qt::white;

//---------------------------------------------------------------------------
//  WordTableDelegate
//
//! Constructor.
//
//! @param parent the parent object
//---------------------------------------------------------------------------
WordTableDelegate::WordTableDelegate (QWidget* parent)
    : QItemDelegate (parent)
{
}

//---------------------------------------------------------------------------
//  sizeHint
//
//! Determine the size hint of an item in a word list.
//
//! @param painter the painter to use
//! @param option style option
//! @param index the model index corresponding to the item
//---------------------------------------------------------------------------
QSize
WordTableDelegate::sizeHint (const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    QString text = index.model()->data (index, Qt::DisplayRole).toString();
    QFontMetrics fm (option.font);
    QRect rect = fm.boundingRect (QRect (0, 0, 1000, 1000), 0, text);
    return QSize (rect.width() + ITEM_XPADDING,
                  rect.height() + ITEM_YPADDING);
}

//---------------------------------------------------------------------------
//  paint
//
//! Paint an item in a word list.
//
//! @param painter the painter to use
//! @param option style option
//! @param index the model index corresponding to the item
//---------------------------------------------------------------------------
void
WordTableDelegate::paint (QPainter* painter, const QStyleOptionViewItem&
                          option, const QModelIndex& index) const
{
    // XXX: Yuck - instead of doing this, the WordTableModel should implement
    // another user-defined role to represent "last-added-ness".
    const WordTableModel* wordModel =
        static_cast<const WordTableModel*>(index.model());
    int lastAddedIndex = wordModel->getLastAddedIndex();

    WordTableModel::WordType type =
        (lastAddedIndex == index.row()) ? WordTableModel::WordLastAdded :
        WordTableModel::WordType (index.model()->data (index,
                                                       Qt::UserRole).toInt());

    QColor color;
    QColor outlineColor;

    // Draw background
    switch (type) {
        case WordTableModel::WordNormal:
        color = VALID_NORMAL_WORD_BACKGROUND;
        outlineColor = VALID_NORMAL_WORD_OUTLINE;
        break;
        case WordTableModel::WordNormalAlternate:
        color = VALID_NORMAL_ALTERNATE_BACKGROUND;
        outlineColor = VALID_NORMAL_ALTERNATE_OUTLINE;
        break;
        case WordTableModel::WordCorrect:
        color = VALID_CORRECT_WORD_BACKGROUND;
        outlineColor = VALID_CORRECT_WORD_OUTLINE;
        break;
        case WordTableModel::WordMissed:
        color = VALID_MISSED_WORD_BACKGROUND;
        outlineColor = VALID_MISSED_WORD_OUTLINE;
        break;
        case WordTableModel::WordIncorrect:
        color = INVALID_WORD_BACKGROUND;
        outlineColor = INVALID_WORD_OUTLINE;
        break;
        case WordTableModel::WordLastAdded:
        color = LAST_ADDED_WORD_BACKGROUND;
        outlineColor = LAST_ADDED_WORD_OUTLINE;
        break;
        default: break;
    }
    painter->setBrush (QBrush (color));
    painter->setPen (color);
    painter->drawRect (option.rect);

    painter->setPen (outlineColor);
    painter->drawLine (option.rect.left(), option.rect.bottom(),
                       option.rect.right(), option.rect.bottom());

    // Draw foreground
    switch (type) {
        case WordTableModel::WordNormal:
        color = VALID_NORMAL_WORD_FOREGROUND;
        break;
        case WordTableModel::WordNormalAlternate:
        color = VALID_NORMAL_ALTERNATE_FOREGROUND;
        break;
        case WordTableModel::WordCorrect:
        color = VALID_CORRECT_WORD_FOREGROUND;
        break;
        case WordTableModel::WordMissed:
        color = VALID_MISSED_WORD_FOREGROUND;
        break;
        case WordTableModel::WordIncorrect:
        color = INVALID_WORD_FOREGROUND;
        break;
        case WordTableModel::WordLastAdded:
        color = LAST_ADDED_WORD_FOREGROUND;
        break;
        default: break;
    }
    painter->setPen (color);

    // Align text to left except for front hooks, which are aligned right
    int flags = Qt::AlignVCenter;
    switch (index.column()) {
        case WordTableModel::FRONT_HOOK_COLUMN:
        case WordTableModel::PROBABILITY_ORDER_COLUMN:
        flags |= Qt::AlignRight;
        break;

        default:
        flags |= Qt::AlignLeft;
        break;
    }

    QRect rectToDraw = option.rect.adjusted (ITEM_XPADDING, 0,
                                             -ITEM_XPADDING, 0);
    int width = option.rect.width() - 2 * ITEM_XPADDING;
    QFontMetrics fm (option.font);
    QString text = index.model()->data (index, Qt::DisplayRole).toString();
    QStringList defs = text.split ("\n");
    QString textToDraw;
    QString def;
    foreach (def, defs) {
        if (!textToDraw.isEmpty())
            textToDraw += "\n";
        textToDraw += elidedText (fm, width, Qt::ElideRight, def);
    }
    painter->drawText (rectToDraw, flags, textToDraw);
}
