//---------------------------------------------------------------------------
// CrosswordGameForm.h
//
// A form for playing a crossword game.
//
// Copyright 2006 Michael W Thelen <mthelen@gmail.com>.
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

#ifndef ZYZZYVA_CROSSWORD_GAME_FORM_H
#define ZYZZYVA_CROSSWORD_GAME_FORM_H

#include "ActionForm.h"
#include "CrosswordGameMove.h"
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QString>

class CrosswordGameBoard;
class CrosswordGameBoardWidget;
class IscConnectionThread;

class CrosswordGameForm : public ActionForm
{
    Q_OBJECT
    public:
    CrosswordGameForm (QWidget* parent = 0, Qt::WFlags f = 0);
    ~CrosswordGameForm();
    QString getStatusString() const;

    public slots:
    void connectClicked();
    void disconnectClicked();
    void inputReturnPressed();
    void threadStatusChanged (const QString& status);
    void threadMessageReceived (const QString& message);

    private:
    void messageAppendHtml (const QString& text, const QColor& color);
    QString encodeHtmlEntities (const QString& text);
    QString canonizeMessage (const QString& text);

    void displayMove (const CrosswordGameMove& move);

    private:
    CrosswordGameBoardWidget* boardWidget;
    QPushButton* connectButton;
    QPushButton* disconnectButton;
    QTextEdit* messageArea;
    QLineEdit* inputLine;
    QString statusString;

    CrosswordGameBoard* board;
    IscConnectionThread* iscThread;
};

#endif // ZYZZYVA_CROSSWORD_GAME_FORM_H