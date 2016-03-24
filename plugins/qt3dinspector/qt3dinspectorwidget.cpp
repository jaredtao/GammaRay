/*
  qt3dinspectorwidget.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Volker Krause <volker.krause@kdab.com>

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

#include "qt3dinspectorwidget.h"
#include "ui_qt3dinspectorwidget.h"
#include "qt3dinspectorclient.h"

#include <common/objectbroker.h>

using namespace GammaRay;

static QObject *create3DInsepctorClient(const QString &/*name*/, QObject *parent)
{
    return new Qt3DInspectorClient(parent);
}

Qt3DInspectorWidget::Qt3DInspectorWidget(QWidget* parent):
    QWidget(parent),
    ui(new Ui::Qt3DInspectorWidget)
{
    ObjectBroker::registerClientObjectFactoryCallback<Qt3DInspectorInterface*>(create3DInsepctorClient);
    m_interface = ObjectBroker::object<Qt3DInspectorInterface*>();

    ui->setupUi(this);
    ui->engineComboBox->setModel(ObjectBroker::model(QStringLiteral("com.kdab.GammaRay.Qt3DInspector.engineModel")));
    connect(ui->engineComboBox, SIGNAL(currentIndexChanged(int)), m_interface, SLOT(selectEngine(int)));

    auto sceneModel = ObjectBroker::model(QStringLiteral("com.kdab.GammaRay.Qt3DInspector.sceneModel"));
    ui->sceneTreeView->setModel(sceneModel);
    ui->sceneTreeView->setSelectionModel(ObjectBroker::selectionModel(sceneModel));

    ui->scenePropertyWidget->setObjectBaseName(QStringLiteral("com.kdab.GammaRay.Qt3DInspector.entityPropertyController"));
}

Qt3DInspectorWidget::~Qt3DInspectorWidget()
{
}
