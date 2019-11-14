/*
 *  qmlsupport.h
 *
 *  This file is part of GammaRay, the Qt application inspection and
 *  manipulation tool.
 *
 *  Copyright (C) 2014-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
 *  Author: Volker Krause <volker.krause@kdab.com>
 *
 *  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
 *  accordance with GammaRay Commercial License Agreement provided with the Software.
 *
 *  Contact info@kdab.com if any conditions of this licensing are not clear to you.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMMARAY_QUICKINSPECTOR_SCENEGRAPHWRAPPER_H
#define GAMMARAY_QUICKINSPECTOR_SCENEGRAPHWRAPPER_H

#include "core/objectwrapper.h"

#include <QQuickWindow>
#include <QQuickView>
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QSGTexture>
#include <QSGNode>
#include <QSGFlatColorMaterial>
#include <QSGTextureMaterial>
#include <QSGVertexColorMaterial>

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
#include <QSGRenderNode>
#include <QSGRendererInterface>
#ifndef QT_NO_OPENGL
#include <private/qquickopenglshadereffectnode_p.h>
#endif
#include <private/qsgsoftwarecontext_p.h>
#include <private/qsgsoftwarerenderer_p.h>
#include <private/qsgsoftwarerenderablenode_p.h>
#endif


#include <private/qquickanchors_p.h>
#include <private/qquickitem_p.h>
#ifndef QT_NO_OPENGL
#include <private/qsgbatchrenderer_p.h>
#endif
#include <private/qsgdistancefieldglyphnode_p_p.h>
#include <private/qabstractanimation_p.h>


Q_DECLARE_METATYPE(QQmlError)

Q_DECLARE_METATYPE(QQuickItem::Flags)
Q_DECLARE_METATYPE(QQuickPaintedItem::PerformanceHints)
Q_DECLARE_METATYPE(QSGNode *)
Q_DECLARE_METATYPE(QSGBasicGeometryNode *)
Q_DECLARE_METATYPE(QSGGeometryNode *)
Q_DECLARE_METATYPE(QSGClipNode *)
Q_DECLARE_METATYPE(QSGTransformNode *)
Q_DECLARE_METATYPE(QSGRootNode *)
Q_DECLARE_METATYPE(QSGOpacityNode *)
Q_DECLARE_METATYPE(QSGNode::Flags)
Q_DECLARE_METATYPE(QSGNode::DirtyState)
Q_DECLARE_METATYPE(QSGGeometry *)
Q_DECLARE_METATYPE(QMatrix4x4 *)
Q_DECLARE_METATYPE(const QMatrix4x4 *)
Q_DECLARE_METATYPE(const QSGClipNode *)
Q_DECLARE_METATYPE(const QSGGeometry *)
Q_DECLARE_METATYPE(QSGMaterial *)
Q_DECLARE_METATYPE(QSGMaterial::Flags)
Q_DECLARE_METATYPE(QSGTexture::WrapMode)
Q_DECLARE_METATYPE(QSGTexture::Filtering)
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
Q_DECLARE_METATYPE(QSGTexture::AnisotropyLevel)
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
Q_DECLARE_METATYPE(QSGRenderNode *)
Q_DECLARE_METATYPE(QSGRenderNode::RenderingFlags)
Q_DECLARE_METATYPE(QSGRenderNode::StateFlags)
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
Q_DECLARE_METATYPE(QSGRendererInterface*)
Q_DECLARE_METATYPE(QSGRendererInterface::GraphicsApi)
Q_DECLARE_METATYPE(QSGRendererInterface::ShaderCompilationTypes)
Q_DECLARE_METATYPE(QSGRendererInterface::ShaderSourceTypes)
Q_DECLARE_METATYPE(QSGRendererInterface::ShaderType)
#endif

Q_DECLARE_METATYPE(QSGTextureProvider*)

Q_DECLARE_METATYPE(QVector<QSGTextureProvider*>)
Q_DECLARE_METATYPE(QVector<QByteArray>)
Q_DECLARE_METATYPE(QVector<QSGNode::NodeType>)

Q_DECLARE_METATYPE(QSGNode::NodeType)


DECLARE_OBJECT_WRAPPER(QWindow) // TODO
DECLARE_OBJECT_WRAPPER(QObject) // TODO

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
DECLARE_OBJECT_WRAPPER(QSGRendererInterface,
    RO_PROP(graphicsApi, Getter)
    RO_PROP(shaderCompilationType, Getter)
    RO_PROP(shaderSourceType, Getter)
    RO_PROP(shaderType, Getter)
)
#endif


DECLARE_OBJECT_WRAPPER(QSGNode,
                       RO_PROP(parent, Getter | NonOwningPointer)
                       RO_PROP(childCount, Getter)
                       RO_PROP(flags, Getter)
                       RO_PROP(isSubtreeBlocked, Getter)
                       RW_PROP(dirtyState, markDirty, Getter)

                       RO_PROP(firstChild, Getter | OwningPointer)
                       RO_PROP(nextSibling, Getter | OwningPointer)

                       RO_PROP(type, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGTransformNode, QSGNode,
                          RW_PROP(matrix, setMatrix, Getter)
                          RW_PROP(combinedMatrix, setCombinedMatrix, Getter)
)


DECLARE_OBJECT_WRAPPER_WB(QQuickItem, QObject,
                          PRIVATE_CLASS(QQuickItemPrivate)
                          RW_PROP(acceptHoverEvents, setAcceptHoverEvents, Getter)
                          RW_PROP(acceptedMouseButtons, setAcceptedMouseButtons, Getter)
                          RW_PROP(cursor, setCursor, Getter)
                          RW_PROP(filtersChildMouseEvents, setFiltersChildMouseEvents, Getter)
                          RW_PROP(flags, setFlags, Getter)
                          RO_PROP(isFocusScope, Getter)
                          RO_PROP(isTextureProvider, Getter)
                          RW_PROP(keepMouseGrab, setKeepMouseGrab, Getter)
                          RW_PROP(keepTouchGrab, setKeepTouchGrab, Getter)
                          CUSTOM_PROP(nextItemInFocusChain, object->isVisible() ? object->nextItemInFocusChain() : nullptr, CustomCommand | NonConst)
                          CUSTOM_PROP(previousItemInFocusChain, object->isVisible() ? object->nextItemInFocusChain(false) : nullptr, CustomCommand | NonConst)
                          RO_PROP(scopedFocusItem, Getter)
//                           RO_PROP(window, Getter)

                          RO_PROP(childItems, Getter | OwningPointer | QProp)
                          RO_PROP(childrenRect, Getter | NonConst)

                          RO_PROP(itemNodeInstance, DptrMember | OwningPointer) // Explicitly avoid calling priv->itemNode() here, which would create a new node outside the scenegraph's behavior.

                          RW_PROP(isVisible, setVisible, Getter)
                          RW_PROP(opacity, setOpacity, Getter)
                          RW_PROP(clip, setClip, Getter)

                          RW_PROP(width, setWidth, Getter)
                          RW_PROP(height, setHeight, Getter)
                          RW_PROP(size, setSize, Getter)
                          RW_PROP(x, setX, Getter)
                          RW_PROP(y, setY, Getter)
                          RW_PROP(z, setZ, Getter)

                          RW_PROP(parentItem, setParentItem, Getter | NonOwningPointer)
//                           RO_PROP(window, Getter) //FIXME how to solve cyclic properties?


                          DIRECT_ACCESS_METHOD(mapToItem) // TODO is this sufficiently safe?
                          DIRECT_ACCESS_METHOD(mapRectToScene)
                          DIRECT_ACCESS_METHOD(contains)
)


DECLARE_OBJECT_WRAPPER_WB(QQuickWindow, QWindow,
    PRIVATE_CLASS(QQuickWindowPrivate)
    RW_PROP(clearBeforeRendering, setClearBeforeRendering, Getter)
    RO_PROP(effectiveDevicePixelRatio, Getter)
#ifndef QT_NO_OPENGL
    RW_PROP(isPersistentOpenGLContext, setPersistentOpenGLContext, Getter)
#endif
    RW_PROP(isPersistentSceneGraph, setPersistentSceneGraph, Getter)
    RO_PROP(mouseGrabberItem, Getter)
#ifndef QT_NO_OPENGL
    RO_PROP(openglContext, Getter)
#endif
    RO_PROP(renderTargetId, Getter)
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    RO_PROP(rendererInterface, Getter)
#endif
    RO_PROP(contentItem, Getter | OwningPointer)
    RW_PROP(customRenderMode, setCustomRenderMode, DptrMember | NonConst)
    ASYNC_VOID_METHOD(update)
    CUSTOM_PROP(qmlContext, QQmlEngine::contextForObject(object), CustomCommand)

    RO_PROP(width, Getter)
    RO_PROP(height, Getter)
    RO_PROP(size, Getter)
)


DECLARE_OBJECT_WRAPPER_WB(QQuickView, QQuickWindow,
    RO_PROP(engine, Getter)
    RO_PROP(errors, Getter)
    RO_PROP(initialSize, Getter)
    RO_PROP(rootContext, Getter)
    RO_PROP(rootObject, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QQuickPaintedItem, QQuickItem,
    RO_PROP(contentsBoundingRect, Getter)
    RW_PROP(mipmap, setMipmap, Getter)
    RW_PROP(opaquePainting, setOpaquePainting, Getter)
    RW_PROP(performanceHints, setPerformanceHints, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGTexture, QObject,
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    RW_PROP(anisotropyLevel, setAnisotropyLevel, Getter)
#endif
    RW_PROP(filtering, setFiltering, Getter)
    RO_PROP(hasAlphaChannel, Getter)
    RO_PROP(hasMipmaps, Getter)
    RW_PROP(horizontalWrapMode, setHorizontalWrapMode, Getter)
    RO_PROP(isAtlasTexture, Getter)
    RW_PROP(mipmapFiltering, setMipmapFiltering, Getter)
    RO_PROP(normalizedTextureSubRect, Getter)
// crashes without a current GL context
//    RO_PROP(textureId, Getter)
    RO_PROP(textureSize, Getter)
    RW_PROP(verticalWrapMode, setVerticalWrapMode, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGBasicGeometryNode, QSGNode,
    RO_PROP(geometry, Getter) // FIXME: This used to have the MSVC workaround
    RO_PROP(matrix, Getter)
    RO_PROP(clipList, Getter)
)

DECLARE_OBJECT_WRAPPER(QSGMaterial,
                       RO_PROP(flags, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGGeometryNode, QSGBasicGeometryNode,
    RW_PROP(material, setMaterial, Getter)
    RW_PROP(opaqueMaterial, setOpaqueMaterial, Getter)
    RO_PROP(activeMaterial, Getter)
    RW_PROP(renderOrder, setRenderOrder, Getter)
    RW_PROP(inheritedOpacity, setInheritedOpacity, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGClipNode, QSGBasicGeometryNode,
    RW_PROP(isRectangular, setIsRectangular, Getter)
    RW_PROP(clipRect, setClipRect, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGRootNode, QSGNode, )

DECLARE_OBJECT_WRAPPER_WB(QSGOpacityNode, QSGNode,
    RW_PROP(opacity, setOpacity, Getter)
    RW_PROP(combinedOpacity, setCombinedOpacity, Getter)
)

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
DECLARE_OBJECT_WRAPPER_WB(QSGRenderNode, QSGNode,
    RO_PROP(changedStates, Getter)
    RO_PROP(flags, Getter)
    RO_PROP(rect, Getter)
    RO_PROP(inheritedOpacity, Getter)
    RO_PROP(matrix, Getter)
    RO_PROP(clipList, Getter)
)
#endif

DECLARE_OBJECT_WRAPPER_WB(QSGFlatColorMaterial, QSGMaterial,
    RW_PROP(color, setColor, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGOpaqueTextureMaterial, QSGMaterial,
    RW_PROP(filtering, setFiltering, Getter)
    RW_PROP(horizontalWrapMode, setHorizontalWrapMode, Getter)
    RW_PROP(mipmapFiltering, setMipmapFiltering, Getter)
    RW_PROP(texture, setTexture, Getter)
    RW_PROP(verticalWrapMode, setVerticalWrapMode, Getter)
)
DECLARE_OBJECT_WRAPPER_WB(QSGTextureMaterial, QSGOpaqueTextureMaterial,)

DECLARE_OBJECT_WRAPPER_WB(QSGVertexColorMaterial, QSGMaterial,)

#ifndef QT_NO_OPENGL
DECLARE_OBJECT_WRAPPER_WB(QSGDistanceFieldTextMaterial, QSGMaterial,
    RO_PROP(color, Getter)
    RO_PROP(fontScale, Getter)
    RO_PROP(textureSize, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGDistanceFieldStyledTextMaterial, QSGDistanceFieldTextMaterial,
    RO_PROP(styleColor, Getter)
)

DECLARE_OBJECT_WRAPPER_WB(QSGDistanceFieldShiftedStyleTextMaterial, QSGDistanceFieldStyledTextMaterial,
    RO_PROP(shift, Getter)
)

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
DECLARE_OBJECT_WRAPPER_WB(QQuickOpenGLShaderEffectMaterial, QSGMaterial,
    RW_PROP(attributes, setAttributes, MemberVar)
    RW_PROP(cullMode, setCullMode, MemberVar)
    RW_PROP(geometryUsesTextureSubRect, setGeometryUsesTextureSubRect, MemberVar)
    RW_PROP(textureProviders, setTextureProviders, MemberVar)
)
#endif
#endif

namespace GammaRay {

inline ObjectView<QQuickWindow> windowForItem(ObjectView<QQuickItem> item)
{
    return ObjectShadowDataRepository::viewForObject(item.object()->window());
}

}


#endif
