/****************************************************************************
**
** Copyright (C) 2012 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Commercial Charts Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxymodelmapper.h"
#include "qxymodelmapper_p.h"
#include "qxyseries.h"
#include <QAbstractItemModel>
#include <QDateTime>

QTCOMMERCIALCHART_BEGIN_NAMESPACE

/*!
    Constructs a mapper object which is a child of \a parent.
*/
QXYModelMapper::QXYModelMapper(QObject *parent)
    : QObject(parent),
      d_ptr(new QXYModelMapperPrivate(this))
{
}

/*!
    \internal
*/
QAbstractItemModel *QXYModelMapper::model() const
{
    Q_D(const QXYModelMapper);
    return d->m_model;
}

/*!
    \internal
*/
void QXYModelMapper::setModel(QAbstractItemModel *model)
{
    if (model == 0)
        return;

    Q_D(QXYModelMapper);
    if (d->m_model)
        disconnect(d->m_model, 0, d, 0);

    d->m_model = model;
    d->initializeXYFromModel();
    //    connect signals from the model
    connect(d->m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), d, SLOT(modelUpdated(QModelIndex,QModelIndex)));
    connect(d->m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), d, SLOT(modelRowsAdded(QModelIndex,int,int)));
    connect(d->m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), d, SLOT(modelRowsRemoved(QModelIndex,int,int)));
    connect(d->m_model, SIGNAL(columnsInserted(QModelIndex,int,int)), d, SLOT(modelColumnsAdded(QModelIndex,int,int)));
    connect(d->m_model, SIGNAL(columnsRemoved(QModelIndex,int,int)), d, SLOT(modelColumnsRemoved(QModelIndex,int,int)));
    connect(d->m_model, SIGNAL(destroyed()), d, SLOT(handleModelDestroyed()));
}

/*!
    \internal
*/
QXYSeries *QXYModelMapper::series() const
{
    Q_D(const QXYModelMapper);
    return d->m_series;
}

/*!
    \internal
*/
void QXYModelMapper::setSeries(QXYSeries *series)
{
    Q_D(QXYModelMapper);
    if (d->m_series)
        disconnect(d->m_series, 0, d, 0);

    if (series == 0)
        return;

    d->m_series = series;
    d->initializeXYFromModel();
    // connect the signals from the series
    connect(d->m_series, SIGNAL(pointAdded(int)), d, SLOT(handlePointAdded(int)));
    connect(d->m_series, SIGNAL(pointRemoved(int)), d, SLOT(handlePointRemoved(int)));
    connect(d->m_series, SIGNAL(pointReplaced(int)), d, SLOT(handlePointReplaced(int)));
    connect(d->m_series, SIGNAL(destroyed()), d, SLOT(handleSeriesDestroyed()));
}

/*!
    \internal
*/
int QXYModelMapper::first() const
{
    Q_D(const QXYModelMapper);
    return d->m_first;
}

/*!
    \internal
*/
void QXYModelMapper::setFirst(int first)
{
    Q_D(QXYModelMapper);
    d->m_first = qMax(first, 0);
    d->initializeXYFromModel();
}

/*!
    \internal
*/
int QXYModelMapper::count() const
{
    Q_D(const QXYModelMapper);
    return d->m_count;
}

/*!
    \internal
*/
void QXYModelMapper::setCount(int count)
{
    Q_D(QXYModelMapper);
    d->m_count = qMax(count, -1);
    d->initializeXYFromModel();
}

/*!
    Returns the orientation that is used when QXYModelMapper accesses the model.
    This mean whether the consecutive x/y values of the QXYSeries are read from rows (Qt::Horizontal)
    or from columns (Qt::Vertical)
*/
Qt::Orientation QXYModelMapper::orientation() const
{
    Q_D(const QXYModelMapper);
    return d->m_orientation;
}

/*!
    Returns the \a orientation that is used when QXYModelMapper accesses the model.
    This mean whether the consecutive x/y values of the QXYSeries are read from rows (Qt::Horizontal)
    or from columns (Qt::Vertical)
*/
void QXYModelMapper::setOrientation(Qt::Orientation orientation)
{
    Q_D(QXYModelMapper);
    d->m_orientation = orientation;
    d->initializeXYFromModel();
}

/*!
    Returns which section of the model is kept in sync with the x values of the QXYSeries
*/
int QXYModelMapper::xSection() const
{
    Q_D(const QXYModelMapper);
    return d->m_xSection;
}

/*!
    Sets the model section that is kept in sync with the x values of the QXYSeries.
    Parameter \a xSection specifies the section of the model.
*/
void QXYModelMapper::setXSection(int xSection)
{
    Q_D(QXYModelMapper);
    d->m_xSection = qMax(-1, xSection);
    d->initializeXYFromModel();
}

/*!
    Returns which section of the model is kept in sync with the y values of the QXYSeries
*/
int QXYModelMapper::ySection() const
{
    Q_D(const QXYModelMapper);
    return d->m_ySection;
}

/*!
    Sets the model section that is kept in sync with the y values of the QXYSeries.
    Parameter \a ySection specifies the section of the model.
*/
void QXYModelMapper::setYSection(int ySection)
{
    Q_D(QXYModelMapper);
    d->m_ySection = qMax(-1, ySection);
    d->initializeXYFromModel();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QXYModelMapperPrivate::QXYModelMapperPrivate(QXYModelMapper *q) :
    QObject(q),
    m_series(0),
    m_model(0),
    m_first(0),
    m_count(-1),
    m_orientation(Qt::Vertical),
    m_xSection(-1),
    m_ySection(-1),
    m_seriesSignalsBlock(false),
    m_modelSignalsBlock(false),
    q_ptr(q)
{
}

void QXYModelMapperPrivate::blockModelSignals(bool block)
{
    m_modelSignalsBlock = block;
}

void QXYModelMapperPrivate::blockSeriesSignals(bool block)
{
    m_seriesSignalsBlock = block;
}

QModelIndex QXYModelMapperPrivate::xModelIndex(int xPos)
{
    if (m_count != -1 && xPos >= m_count)
        return QModelIndex(); // invalid

    if (m_orientation == Qt::Vertical)
        return m_model->index(xPos + m_first, m_xSection);
    else
        return m_model->index(m_xSection, xPos + m_first);
}

QModelIndex QXYModelMapperPrivate::yModelIndex(int yPos)
{
    if (m_count != -1 && yPos >= m_count)
        return QModelIndex(); // invalid

    if (m_orientation == Qt::Vertical)
        return m_model->index(yPos + m_first, m_ySection);
    else
        return m_model->index(m_ySection, yPos + m_first);
}

qreal QXYModelMapperPrivate::valueFromModel(QModelIndex index)
{
    QVariant value = m_model->data(index, Qt::DisplayRole);
    switch (value.type()) {
    case QVariant::DateTime:
        return value.toDateTime().toMSecsSinceEpoch();
    case QVariant::Date:
        return QDateTime(value.toDate()).toMSecsSinceEpoch();
    default:
        return value.toReal();
    }
}

void QXYModelMapperPrivate::setValueToModel(QModelIndex index, qreal value)
{
    QVariant oldValue = m_model->data(index, Qt::DisplayRole);
    switch (oldValue.type()) {
    case QVariant::DateTime:
        m_model->setData(index, QDateTime::fromMSecsSinceEpoch(value));
        break;
    case QVariant::Date:
        m_model->setData(index, QDateTime::fromMSecsSinceEpoch(value).date());
        break;
    default:
        m_model->setData(index, value);
    }
}

void QXYModelMapperPrivate::handlePointAdded(int pointPos)
{
    if (m_seriesSignalsBlock)
        return;

    if (m_count != -1)
        m_count += 1;

    blockModelSignals();
    if (m_orientation == Qt::Vertical)
        m_model->insertRows(pointPos + m_first, 1);
    else
        m_model->insertColumns(pointPos + m_first, 1);

    setValueToModel(xModelIndex(pointPos), m_series->points().at(pointPos).x());
    setValueToModel(yModelIndex(pointPos), m_series->points().at(pointPos).y());
    blockModelSignals(false);
}

void QXYModelMapperPrivate::handlePointRemoved(int pointPos)
{
    if (m_seriesSignalsBlock)
        return;

    if (m_count != -1)
        m_count -= 1;

    blockModelSignals();
    if (m_orientation == Qt::Vertical)
        m_model->removeRow(pointPos + m_first);
    else
        m_model->removeColumn(pointPos + m_first);
    blockModelSignals(false);
}

void QXYModelMapperPrivate::handlePointReplaced(int pointPos)
{
    if (m_seriesSignalsBlock)
        return;

    blockModelSignals();
    setValueToModel(xModelIndex(pointPos), m_series->points().at(pointPos).x());
    setValueToModel(yModelIndex(pointPos), m_series->points().at(pointPos).y());
    blockModelSignals(false);
}

void QXYModelMapperPrivate::handleSeriesDestroyed()
{
    m_series = 0;
}

void QXYModelMapperPrivate::modelUpdated(QModelIndex topLeft, QModelIndex bottomRight)
{
    if (m_model == 0 || m_series == 0)
        return;

    if (m_modelSignalsBlock)
        return;

    blockSeriesSignals();
    QModelIndex index;
    QPointF oldPoint;
    QPointF newPoint;
    for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
        for (int column = topLeft.column(); column <= bottomRight.column(); column++) {
            index = topLeft.sibling(row, column);
            if (m_orientation == Qt::Vertical && (index.column() == m_xSection || index.column() == m_ySection)) {
                if (index.row() >= m_first && (m_count == - 1 || index.row() < m_first + m_count)) {
                    QModelIndex xIndex = xModelIndex(index.row() - m_first);
                    QModelIndex yIndex = yModelIndex(index.row() - m_first);
                    if (xIndex.isValid() && yIndex.isValid()) {
                        oldPoint = m_series->points().at(index.row() - m_first);
                        newPoint.setX(valueFromModel(xIndex));
                        newPoint.setY(valueFromModel(yIndex));
                    }
                }
            } else if (m_orientation == Qt::Horizontal && (index.row() == m_xSection || index.row() == m_ySection)) {
                if (index.column() >= m_first && (m_count == - 1 || index.column() < m_first + m_count)) {
                    QModelIndex xIndex = xModelIndex(index.column() - m_first);
                    QModelIndex yIndex = yModelIndex(index.column() - m_first);
                    if (xIndex.isValid() && yIndex.isValid()) {
                        oldPoint = m_series->points().at(index.column() - m_first);
                        newPoint.setX(valueFromModel(xIndex));
                        newPoint.setY(valueFromModel(yIndex));
                    }
                }
            } else {
                continue;
            }
            m_series->replace(oldPoint, newPoint);
        }
    }
    blockSeriesSignals(false);
}

void QXYModelMapperPrivate::modelRowsAdded(QModelIndex parent, int start, int end)
{
    Q_UNUSED(parent);
    if (m_modelSignalsBlock)
        return;

    blockSeriesSignals();
    if (m_orientation == Qt::Vertical)
        insertData(start, end);
    else if (start <= m_xSection || start <= m_ySection) // if the changes affect the map - reinitialize the xy
        initializeXYFromModel();
    blockSeriesSignals(false);
}

void QXYModelMapperPrivate::modelRowsRemoved(QModelIndex parent, int start, int end)
{
    Q_UNUSED(parent);
    if (m_modelSignalsBlock)
        return;

    blockSeriesSignals();
    if (m_orientation == Qt::Vertical)
        removeData(start, end);
    else if (start <= m_xSection || start <= m_ySection) // if the changes affect the map - reinitialize the xy
        initializeXYFromModel();
    blockSeriesSignals(false);
}

void QXYModelMapperPrivate::modelColumnsAdded(QModelIndex parent, int start, int end)
{
    Q_UNUSED(parent);
    if (m_modelSignalsBlock)
        return;

    blockSeriesSignals();
    if (m_orientation == Qt::Horizontal)
        insertData(start, end);
    else if (start <= m_xSection || start <= m_ySection) // if the changes affect the map - reinitialize the xy
        initializeXYFromModel();
    blockSeriesSignals(false);
}

void QXYModelMapperPrivate::modelColumnsRemoved(QModelIndex parent, int start, int end)
{
    Q_UNUSED(parent);
    if (m_modelSignalsBlock)
        return;

    blockSeriesSignals();
    if (m_orientation == Qt::Horizontal)
        removeData(start, end);
    else if (start <= m_xSection || start <= m_ySection) // if the changes affect the map - reinitialize the xy
        initializeXYFromModel();
    blockSeriesSignals(false);
}

void QXYModelMapperPrivate::handleModelDestroyed()
{
    m_model = 0;
}

void QXYModelMapperPrivate::insertData(int start, int end)
{
    if (m_model == 0 || m_series == 0)
        return;

    if (m_count != -1 && start >= m_first + m_count) {
        return;
    } else {
        int addedCount = end - start + 1;
        if (m_count != -1 && addedCount > m_count)
            addedCount = m_count;
        int first = qMax(start, m_first);
        int last = qMin(first + addedCount - 1, m_orientation == Qt::Vertical ? m_model->rowCount() - 1 : m_model->columnCount() - 1);
        for (int i = first; i <= last; i++) {
            QPointF point;
            QModelIndex xIndex = xModelIndex(i - m_first);
            QModelIndex yIndex = yModelIndex(i - m_first);
            if (xIndex.isValid() && yIndex.isValid()) {
                point.setX(valueFromModel(xIndex));
                point.setY(valueFromModel(yIndex));
                m_series->insert(i - m_first, point);
            }
        }

        // remove excess of points (above m_count)
        if (m_count != -1 && m_series->points().size() > m_count)
            for (int i = m_series->points().size() - 1; i >= m_count; i--) {
                m_series->remove(m_series->points().at(i));
            }
    }
}

void QXYModelMapperPrivate::removeData(int start, int end)
{
    if (m_model == 0 || m_series == 0)
        return;

    int removedCount = end - start + 1;
    if (m_count != -1 && start >= m_first + m_count) {
        return;
    } else {
        int toRemove = qMin(m_series->count(), removedCount);     // first find how many items can actually be removed
        int first = qMax(start, m_first);    // get the index of the first item that will be removed.
        int last = qMin(first + toRemove - 1, m_series->count() + m_first - 1);    // get the index of the last item that will be removed.
        for (int i = last; i >= first; i--) {
            m_series->remove(m_series->points().at(i - m_first));
        }

        if (m_count != -1) {
            int itemsAvailable;     // check how many are available to be added
            if (m_orientation == Qt::Vertical)
                itemsAvailable = m_model->rowCount() - m_first - m_series->count();
            else
                itemsAvailable = m_model->columnCount() - m_first - m_series->count();
            int toBeAdded = qMin(itemsAvailable, m_count - m_series->count());     // add not more items than there is space left to be filled.
            int currentSize = m_series->count();
            if (toBeAdded > 0)
                for (int i = m_series->count(); i < currentSize + toBeAdded; i++) {
                    QPointF point;
                    QModelIndex xIndex = xModelIndex(i);
                    QModelIndex yIndex = yModelIndex(i);
                    if (xIndex.isValid() && yIndex.isValid()) {
                        point.setX(valueFromModel(xIndex));
                        point.setY(valueFromModel(yIndex));
                        m_series->insert(i, point);
                    }
                }
        }
    }
}

void QXYModelMapperPrivate::initializeXYFromModel()
{
    if (m_model == 0 || m_series == 0)
        return;

    blockSeriesSignals();
    // clear current content
    m_series->clear();

    // create the initial points set
    int pointPos = 0;
    QModelIndex xIndex = xModelIndex(pointPos);
    QModelIndex yIndex = yModelIndex(pointPos);
    while (xIndex.isValid() && yIndex.isValid()) {
        QPointF point;
        point.setX(valueFromModel(xIndex));
        point.setY(valueFromModel(yIndex));
        m_series->append(point);
        pointPos++;
        xIndex = xModelIndex(pointPos);
        yIndex = yModelIndex(pointPos);
    }
    blockSeriesSignals(false);
}

#include "moc_qxymodelmapper.cpp"
#include "moc_qxymodelmapper_p.cpp"

QTCOMMERCIALCHART_END_NAMESPACE
