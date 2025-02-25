﻿/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/
#ifndef RS_ACTIONPOLYLINEEQUIDISTANT_H
#define RS_ACTIONPOLYLINEEQUIDISTANT_H

#include <QSharedPointer>
#include "RVector.h"
#include "REntity.h"

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class ECPolylineEquidistant
{
    Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        ChooseEntity			/**< Choosing the original polyline. */
    };

public:
    ECPolylineEquidistant(RS_EntityContainer& container,
                          RS_GraphicView& graphicView);
    ~ECPolylineEquidistant()=default;

    //static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

    virtual void init(int status=0);

    virtual void trigger();

    //virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    //virtual void updateToolBar();
    virtual void showOptions();
    virtual void hideOptions();

    void setDist(const double& d) {
        m_dDist = d;
    }

    double getDist() const{
        return m_dDist;
    }

    void setNumber(unsigned n) {
        m_nNumber = n;
    }

    int getNumber() const{
        return m_nNumber;
    }

    bool makeContour();

private:
    QSharedPointer<REntity> calculateOffset(QSharedPointer<REntity> newEntity, QSharedPointer<REntity> orgEntity, double dDist);
    RVector calculateIntersection(QSharedPointer<REntity> first, QSharedPointer<REntity> last);

private:
    QSharedPointer<REntity> originalEntity;
    RVector m_targetPoint;
    double m_dDist;
    int m_nNumber;
    bool m_bRightSide;
};

#endif
