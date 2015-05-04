//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. Written by Alfredo
// Gimenez (alfredo.gimenez@gmail.com). LLNL-CODE-663358. All rights
// reserved.
//
// This file is part of MemAxes. For details, see
// https://github.com/scalability-tools/MemAxes
//
// Please also read this link – Our Notice and GNU Lesser General Public
// License. This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License (as
// published by the Free Software Foundation) version 2.1 dated February
// 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// OUR NOTICE AND TERMS AND CONDITIONS OF THE GNU GENERAL PUBLIC LICENSE
// Our Preamble Notice
// A. This notice is required to be provided under our contract with the
// U.S. Department of Energy (DOE). This work was produced at the Lawrence
// Livermore National Laboratory under Contract No. DE-AC52-07NA27344 with
// the DOE.
// B. Neither the United States Government nor Lawrence Livermore National
// Security, LLC nor any of their employees, makes any warranty, express or
// implied, or assumes any liability or responsibility for the accuracy,
// completeness, or usefulness of any information, apparatus, product, or
// process disclosed, or represents that its use would not infringe
// privately-owned rights.
//////////////////////////////////////////////////////////////////////////////

#include "axisvizwidget.h"

#include <QPaintEvent>
#include <QMenu>

#include <iostream>
#include <algorithm>

#include "util.h"

AxisVizWidget::AxisVizWidget(QWidget *parent)
    : VizWidget(parent)
{
    // Defaults
    dim = 0;
    clusterDepth = 0;
    numHistBins = 100;
    numMetricBins = 60;

    drawHists = 1;
    drawClusters = 1;
    drawMetrics = 1;

    clusterIndex = -1;

    needsCalcMinMaxes = false;
    needsCalcHistBins = false;
    needsRepaint = false;
    needsResizeClusters = false;

    metric_type = CORE_IMBALANCE;
    metricColorMap = gradientColorMap(QColor(254,224,210),QColor(222,45,38),256);

    installEventFilter(this);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)));
}

void AxisVizWidget::frameUpdate()
{
    if(needsCalcMinMaxes)
    {
        calcMinMax();
        needsCalcHistBins = true;
        needsCalcMinMaxes = false;
    }
    if(needsCalcHistBins)
    {
        calcHistBins();
        needsRepaint = true;
        needsCalcHistBins = false;
    }
    if(needsGatherClusters)
    {
        //gatherClusters();
        needsResizeClusters = true;
        needsGatherClusters = false;
    }
    if(needsCalcMetrics)
    {
        gatherMetricBins();
        calcMetrics();
        needsRepaint = true;
        needsCalcMetrics = false;
    }
    if(needsResizeClusters)
    {
        resizeClusters();
        needsRepaint = true;
        needsResizeClusters = false;
    }
    if(needsRepaint)
    {
        repaint();
        needsRepaint = false;
    }
}

void AxisVizWidget::selectionChangedSlot()
{
    needsCalcHistBins = true;
}

void AxisVizWidget::visibilityChangedSlot()
{
    needsCalcMinMaxes = true;
}

void AxisVizWidget::showContextMenu(const QPoint &pos)
{
    QMenu contextMenu(tr("Axis Menu"), this);

    QAction actionAnimate("Animate", this);
    connect(&actionAnimate, SIGNAL(triggered()), this, SLOT(beginAnimation()));
    contextMenu.addAction(&actionAnimate);

    contextMenu.exec(mapToGlobal(pos));

}

void AxisVizWidget::setDimension(int d)
{
    dim = d;
    needsCalcMetrics = true;
    needsCalcMinMaxes = true;
}

void AxisVizWidget::setNumBins(int d)
{
    numHistBins = d;
    needsCalcHistBins = true;
}

void AxisVizWidget::setClusterDepth(int d)
{
    clusterDepth = d;
    needsGatherClusters = true;
}

void AxisVizWidget::setShowHistograms(bool checked)
{

}

void AxisVizWidget::activateClusters()
{
    needsGatherClusters = true;
}

void AxisVizWidget::setDrawHists(int on)
{
    drawHists = on;
    needsRepaint = true;
}

void AxisVizWidget::setDrawClusters(int on)
{
    drawClusters = on;
    needsRepaint = true;
}

void AxisVizWidget::setDrawMetrics(int on)
{
    drawMetrics = on;
    needsRepaint = true;
}

void AxisVizWidget::setMetric(int type)
{
    metric_type = (METRIC_TYPE)type;
    needsCalcMetrics = true;
    needsResizeClusters = true;
}

void AxisVizWidget::beginAnimation()
{

}

void AxisVizWidget::endAnimation()
{

}

void AxisVizWidget::resizeEvent(QResizeEvent *e)
{
    int mx=40;
    int my=30;

    plotBBox = QRectF(mx,my,
                      width()-mx-mx,
                      height()-my-my);

    needsResizeClusters = true;
    needsRepaint = true;
}

void AxisVizWidget::processData()
{
    needsCalcMinMaxes = true;
    needsCalcMetrics = true;
    processed = true;
}

void AxisVizWidget::drawQtPainter(QPainter *painter)
{
    if(!processed)
        return;

    qreal metric_height;

    if(drawMetrics)
        metric_height = 30;
    else
        metric_height = 0;

    // Draw axes
    painter->setPen(Qt::black);
    painter->setBrush(Qt::NoBrush);

    QPointF a = plotBBox.bottomLeft()+QPoint(0,-metric_height);
    QPointF b = plotBBox.bottomRight();

    QFontMetrics fm = painter->fontMetrics();
    painter->drawRect(QRectF(a,b));

    QString text = dataSet->meta.at(dim);
    QPointF center = plotBBox.topLeft();
    painter->drawText(center,text);

    text = QString::number(dimMin,'g',2);
    center = a + QPointF(0,metric_height+fm.height());
    painter->drawText(center,text);

    text = QString::number(dimMax,'g',2);
    center = b + QPointF(-fm.width(text),fm.height());
    painter->drawText(center,text);

    if(drawHists)
    {
        // Draw histograms
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(31,120,180));

        qreal left = plotBBox.left();
        qreal histHeight = plotBBox.height() - metric_height;
        qreal histWidth = plotBBox.width() / numHistBins;
        for(int b=0; b<numHistBins; b++)
        {
            qreal height = histVals.at(b)*histHeight;
            qreal top = plotBBox.top()+(histHeight - height);
            QRectF histRect(left,top,histWidth,height);
            left += histWidth;

            painter->drawRect(histRect);
        }
    }

    if(0)//drawMetrics)
    {
        // Draw metrics
        painter->setPen(Qt::black);

        qreal top = plotBBox.bottom()-metric_height;
        qreal left = plotBBox.left();
        qreal metricHeight = metric_height;
        qreal metricWidth = plotBBox.width() / numMetricBins;
        for(int b=0; b<numMetricBins; b++)
        {
            qreal val = scale(metricVals.at(b),0,metricMax,0,1);
            painter->setBrush(valToColor(val,metricColorMap));

            QRectF histRect(left,top,metricWidth,metricHeight);
            left += metricWidth;

            painter->drawRect(histRect);
        }
    }

    if(drawClusters)
    {
        // Draw cluster aggregates
        for(unsigned int i=0; i<clusterAggregates.size(); i++)
        {
            topoBox *t = &clusterAggregates.at(i);

            painter->setPen(QColor(0,0,0));
            painter->setBrush(t->color);
            QRectF b = t->box;
            painter->drawEllipse(b);

            if(t->drawGlyph)
            {
                t->htp.draw(painter);
            }
        }
    }
}

void AxisVizWidget::leaveEvent(QEvent *e)
{

}

void AxisVizWidget::mousePressEvent(QMouseEvent *e)
{

}

void AxisVizWidget::mouseReleaseEvent(QMouseEvent *e)
{
    for(unsigned int b=0; b<clusterAggregates.size(); b++)
    {
        topoBox *t = &clusterAggregates.at(b);
        if(t->box.contains(e->pos()))
        {
            ElemSet topoSamples = t->htp.getTopo()->getAllSamples();
            dataSet->selectSet(topoSamples);
            emit selectionChangedSig();
            return;
        }
    }
}

bool AxisVizWidget::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QPoint mousePos = mouseEvent->pos();

    if(event->type() == QEvent::MouseMove)
    {
        for(unsigned int b=0; b<clusterAggregates.size(); b++)
        {
            int topoGlyphSize = 260;
            topoBox *t = &clusterAggregates.at(b);
            if(t->box.contains(mousePos))
            {
                qreal left = t->box.center().x()-topoGlyphSize/2;
                left = std::max(left,plotBBox.left());
                left = std::min(left,plotBBox.right()-topoGlyphSize);
                qreal top = plotBBox.bottom()-topoGlyphSize-30;

                QRectF topoGlyphBox(left,top,
                                    topoGlyphSize,topoGlyphSize);
                t->htp.resize(topoGlyphBox);
                t->drawGlyph = true;
            }
            else
            {
                t->drawGlyph = false;
            }
        }
        needsRepaint = true;
    }

    return false;
}

void AxisVizWidget::calcMinMax()
{
    if(!processed)
        return;

    dimMin = std::numeric_limits<double>::max();
    dimMax = std::numeric_limits<double>::min();

    // Get min and max of visible elements in this dimension
    for(int elem=0; elem<dataSet->numElements; elem++)
    {
        if(!dataSet->visible(elem))
            continue;

        dimMin = std::min(dimMin,dataSet->at(elem,dim));
        dimMax = std::max(dimMax,dataSet->at(elem,dim));
    }
}

void AxisVizWidget::calcHistBins()
{
    if(!processed)
        return;

    // Select histogram bin and increment
    // Gather maximum sized bin along the way
    histMax = 0;
    histVals.resize(numHistBins,0);
    for(int elem=0; elem<dataSet->numElements; elem++)
    {
        if(dataSet->selectionDefined() && !dataSet->selected(elem))
            continue;

        int histBin = getHistBin(dataSet->at(elem,dim),
                                 dataSet->minAt(dim),
                                 dataSet->maxAt(dim),
                                 numHistBins);

        histVals[histBin] += 1;
        histMax = std::max(histMax,histVals.at(histBin));
    }

    // Scale hist values to [0,1]
    for(int j=0; j<numHistBins; j++)
        histVals[j] = scale(histVals.at(j),0,histMax,0,1);
}

void AxisVizWidget::calcMetrics()
{
    if(!processed)
        return;

    metricMax = 0;
    metricVals.resize(numMetricBins,0);
    for(int b=0; b<numMetricBins; b++)
    {
        metricVals[b] = binAggs[b].getMetric(metric_type);
        metricMax = std::max(metricMax,metricVals.at(b));
    }
}

void AxisVizWidget::gatherMetricBins()
{
    if(!processed)
        return;

    bins.clear();
    binAggs.clear();

    bins.resize(numMetricBins);
    binAggs.resize(numMetricBins);

    for(int elem=0; elem<dataSet->numElements; elem++)
    {
        int metricBin = getHistBin(dataSet->at(elem,dim),
                                   dataSet->minAt(dim),
                                   dataSet->maxAt(dim),
                                   numMetricBins);
        bins[metricBin].insert(elem);
    }


    for(int b=0; b<numMetricBins; b++)
    {
        binAggs[b].createAggregateFromSamples(dataSet,&bins.at(b));
    }
}

void AxisVizWidget::gatherClusters()
{
    if(!processed)
        return;

    clusterAggregates.clear();

    DataClusterTree *tree = dataSet->clusterTrees.at(dim);

    if(!tree)
        return;

    std::vector<DataClusterNode*> depthNodes = tree->getNodesAtDepth(clusterDepth);

    for(unsigned int i=0; i<depthNodes.size(); i++)
    {
        // getNodesAtDepth only returns internal nodes
        DataClusterInternalNode *inode = (DataClusterInternalNode*)depthNodes.at(i);

        // Create topo box
        topoBox tb;
        tb.agg = (HardwareClusterAggregate*)inode->aggregate;
        tb.htp = HWTopoPainter(tb.agg->getTopo());
        tb.htp.calcMinMaxes();
        tb.minRange = inode->rangeMin;
        tb.maxRange = inode->rangeMax;
        tb.drawGlyph = false;

        clusterAggregates.push_back(tb);
    }
}

void AxisVizWidget::resizeClusters()
{
    qreal minMetric = std::numeric_limits<qreal>::max();
    qreal maxMetric = std::numeric_limits<qreal>::min();

    for(unsigned int i=0; i<clusterAggregates.size(); i++)
    {
        topoBox *tb = &clusterAggregates.at(i);

        // Make box at center of range
        qreal topoWidth = 20;
        qreal drawLeft = scale(tb->minRange,dataSet->minAt(dim),dataSet->maxAt(dim),plotBBox.left(),plotBBox.right());
        tb->box = QRect(drawLeft,plotBBox.bottom()-topoWidth/2,
                        topoWidth,topoWidth);

        // Get metric
        qreal metric = tb->agg->getMetric(metric_type);

        // Min and max for metric
        minMetric = std::min(minMetric,metric);
        maxMetric = std::max(maxMetric,metric);
    }

    for(unsigned int i=0; i<clusterAggregates.size(); i++)
    {
        // Color
        topoBox *tb = &clusterAggregates.at(i);
        qreal metric = tb->agg->getMetric(metric_type);
        qreal val = scale(metric,minMetric,maxMetric,0,1);
        tb->color = valToColor(val,tb->htp.getColorMap());
    }
}