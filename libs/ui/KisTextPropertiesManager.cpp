/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisTextPropertiesManager.h"
#include <KoSelectedShapesProxy.h>
#include <kis_canvas_resource_provider.h>
#include <KoSvgTextPropertyData.h>
#include <KoSelection.h>
#include <KoSvgTextShape.h>
#include <KoSvgTextPropertiesInterface.h>

#include <KisView.h>
#include <kis_canvas2.h>

#include <QPointer>

struct KisTextPropertiesManager::Private {
    KisCanvasResourceProvider *provider {nullptr};
    QPointer<KisView> view {nullptr};
    KoSvgTextPropertiesInterface *interface {nullptr};
};

KisTextPropertiesManager::KisTextPropertiesManager(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

KisTextPropertiesManager::~KisTextPropertiesManager()
{
}

void KisTextPropertiesManager::setView(QPointer<KisView> imageView)
{
    //if (d->view) {
    //    disconnect(d->view->canvasBase()->selectedShapesProxy(), SIGNAL(selectionChanged()), this, SLOT(slotShapeSelectionChanged()));
    //}

    d->view = imageView;
    //if (d->view) {
    //    connect(d->view->canvasBase()->selectedShapesProxy(), SIGNAL(selectionChanged()), this, SLOT(slotShapeSelectionChanged()));
    //}
}

void KisTextPropertiesManager::setCanvasResourceProvider(KisCanvasResourceProvider *provider)
{
    if (d->provider) {
        disconnect(d->provider, SIGNAL(sigTextPropertiesChanged()), this, SLOT(slotTextPropertiesChanged()));
    }
    d->provider = provider;
    if (d->provider) {
        connect(d->provider, SIGNAL(sigTextPropertiesChanged()), this, SLOT(slotTextPropertiesChanged()));
    }
}

void KisTextPropertiesManager::setTextPropertiesInterface(KoSvgTextPropertiesInterface *interface)
{
    if (d->interface) {
        disconnect(d->interface, SIGNAL(textSelectionChanged()), this, SLOT(slotInterfaceSelectionChanged()));
    }
    d->interface = interface;
    if (d->interface) {
        connect(d->interface, SIGNAL(textSelectionChanged()), this, SLOT(slotInterfaceSelectionChanged()));
    }
}

KoSvgTextPropertyData textDataProperties(QList<KoSvgTextProperties> props, QSet<KoSvgTextProperties::PropertyId> propIds) {
    KoSvgTextPropertyData textData;
    textData.commonProperties = props.first();

    for (auto it = props.begin(); it != props.end(); it++) {
        for (int i = 0; i < propIds.size(); i++) {
            KoSvgTextProperties::PropertyId p = propIds.values().at(i);
            if (it->hasProperty(p)) {
                if (textData.commonProperties.property(p) != it->property(p)) {
                    textData.commonProperties.removeProperty(p);
                    textData.tristate.insert(p);
                }
            } else {
                textData.commonProperties.removeProperty(p);
                textData.tristate.insert(p);
            }
        }
    }
    return textData;
}

void KisTextPropertiesManager::slotShapeSelectionChanged()
{
    if (!d->view
            || !d->view->canvasBase()
            || !d->view->canvasBase()->selectedShapesProxy()
            || !d->view->canvasBase()->selectedShapesProxy()->selection()
            || !d->provider) return;



    const QList<KoShape*> shapes = d->view->canvasBase()->selectedShapesProxy()->selection()->selectedEditableShapes();
    QList<KoSvgTextProperties> props;
    QSet<KoSvgTextProperties::PropertyId> propIds;

    for (auto it = shapes.begin(); it != shapes.end(); it++) {
        KoSvgTextShape *textShape = dynamic_cast<KoSvgTextShape*>(*it);
        if (!textShape) continue;
        KoSvgTextProperties p = textShape->textProperties();
        props.append(p);
        for (int i = 0; i < p.properties().size(); i++) {
            propIds.insert(p.properties().at(i));
        }
    }

    if (props.isEmpty()) return;

    // Find common properties, and mark all non-common properties as tristate.

    KoSvgTextPropertyData textData = textDataProperties(props, propIds);

    d->provider->setTextPropertyData(textData);
}

void KisTextPropertiesManager::slotInterfaceSelectionChanged() {
    if (!d->interface || !d->provider) return;

    QList<KoSvgTextProperties> props = d->interface->getSelectedProperties();
    if (props.isEmpty()) return;

    QSet<KoSvgTextProperties::PropertyId> propIds;

    for (auto it = props.begin(); it != props.end(); it++) {
        KoSvgTextProperties p = *it;
        for (int i = 0; i < p.properties().size(); i++) {
            propIds.insert(p.properties().at(i));
        }
    }

    KoSvgTextPropertyData textData = textDataProperties(props, propIds);

    d->provider->setTextPropertyData(textData);

}

void KisTextPropertiesManager::slotTextPropertiesChanged()
{
    if (!d->interface || !d->provider) return;

    d->interface->setPropertiesOnSelected(d->provider->textPropertyData().commonProperties);
}
