/*
  quickscenegraphmodel.h

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2014-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Anton Kreuzkamp <anton.kreuzkamp@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAMMARAY_QUICKINSPECTOR_QUICKSCENEGRAPHMODEL_H
#define GAMMARAY_QUICKINSPECTOR_QUICKSCENEGRAPHMODEL_H

#include <config-gammaray.h>

#include "core/objectmodelbase.h"

#include <QHash>
#include <QPointer>
#include <QVector>

#include "scenegraphwrapper.h"

QT_BEGIN_NAMESPACE
class QSGNode;
class QQuickItem;
class QQuickWindow;
QT_END_NAMESPACE

namespace GammaRay {

/** QQ2 scene graph model. */
class QuickSceneGraphModel : public ObjectModelBase<QAbstractItemModel>
{
    Q_OBJECT
public:
    explicit QuickSceneGraphModel(QObject *parent = nullptr);
    ~QuickSceneGraphModel() override;

    void setWindow(ObjectHandle<QQuickWindow> window);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex indexForNode(ObjectView<QSGNode> node) const;
    ObjectView<QSGNode> sgNodeForItem(ObjectView<QQuickItem> item) const;
    ObjectView<QQuickItem> itemForSgNode(ObjectView<QSGNode> node) const;
    bool verifyNodeValidity(ObjectView<QSGNode> node);

signals:
    void nodeDeleted(ObjectView<QSGNode> node);

private slots:
    void updateSGTree(bool emitSignals = true);

private:
    void clear();
    ObjectHandle<QSGNode> currentRootNode() const;
    void populateFromNode(ObjectView<QSGNode> node, bool emitSignals);
    void collectItemNodes(ObjectView<QQuickItem> item);
    bool recursivelyFindChild(ObjectView<QSGNode> root, ObjectView<QSGNode> child) const;
    void pruneSubTree(ObjectView<QSGNode> node);

    ObjectView<QSGNode> nodeForIndex(const QModelIndex &index) const;

    ObjectHandle<QQuickWindow> m_window;

    ObjectHandle<QSGNode> m_rootNode;
    QHash<ObjectView<QSGNode>, ObjectView<QSGNode>> m_childParentMap;
    QHash<ObjectView<QSGNode>, QVector<ObjectView<QSGNode>> > m_parentChildMap;
    QHash<ObjectView<QQuickItem> , ObjectView<QSGNode> > m_itemItemNodeMap;
    QHash<ObjectView<QSGNode> , ObjectView<QQuickItem> > m_itemNodeItemMap;
};
}

#endif // GAMMARAY_QUICKSCENEGRAPHMODEL_H
