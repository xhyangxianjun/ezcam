﻿/** @file MaxRectsBinPack.cpp
    @author Jukka Jyl鋘ki

    @brief Implements different bin packer algorithms that use the MAXRECTS data structure.

    This work is released to Public Domain, do whatever you want with it.
*/
#include <utility>
#include <iostream>
#include <limits>

#include <cassert>
#include <cstring>
#include <cmath>

#include "MaxRectsBinPack.h"

namespace rbp {

using namespace std;

MaxRectsBinPack::MaxRectsBinPack()
    :binWidth(0),
      binHeight(0)
{
}

MaxRectsBinPack::MaxRectsBinPack(int width, int height)
{
    Init(width, height);
}

void MaxRectsBinPack::Init(int width, int height)
{
    binWidth = width;
    binHeight = height;

    Rect n;
    n.x = 0;
    n.y = 0;
    n.width = width;
    n.height = height;

    usedRectangles.clear();

    freeRectangles.clear();
    freeRectangles.push_back(n);
}

Rect MaxRectsBinPack::Insert(const RectSize &rcSz, FreeRectChoiceHeuristic method)
{
    Rect newNode;
    // Unused in this function. We don't need to know the score after finding the position.
    int score1 = std::numeric_limits<int>::max();
    int score2 = std::numeric_limits<int>::max();
    switch(method)
    {
    case RectBestShortSideFit:
        newNode = FindPositionForNewNodeBestShortSideFit(rcSz, score1, score2);
        break;
    case RectBottomLeftRule:
        newNode = FindPositionForNewNodeBottomLeft(rcSz, score1, score2);
        break;
    case RectContactPointRule:
        newNode = FindPositionForNewNodeContactPoint(rcSz, score1);
        break;
    case RectBestLongSideFit:
        newNode = FindPositionForNewNodeBestLongSideFit(rcSz, score2, score1);
        break;
    case RectBestAreaFit:
        newNode = FindPositionForNewNodeBestAreaFit(rcSz, score1, score2);
        break;
    }

    if (newNode.height == 0)
        return newNode;

    size_t numRectanglesToProcess = freeRectangles.size();
    for(size_t i = 0; i < numRectanglesToProcess; ++i)
    {
        if (SplitFreeNode(freeRectangles[i], newNode))
        {
            freeRectangles.erase(freeRectangles.begin() + i);
            --i;
            --numRectanglesToProcess;
        }
    }

    PruneFreeList();

    usedRectangles.push_back(newNode);
    return newNode;
}

void MaxRectsBinPack::Insert(std::vector<RectSize> &rects, FreeRectChoiceHeuristic method)
{
    while(rects.size() > 0)
    {
        int bestScore1 = std::numeric_limits<int>::max();
        int bestScore2 = std::numeric_limits<int>::max();
        int bestRectIndex = -1;
        Rect bestNode;

        for(size_t i = 0; i < rects.size(); ++i)
        {
            int score1;
            int score2;
            Rect newNode = ScoreRect(rects[i], method, score1, score2);

            if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2))
            {
                bestScore1 = score1;
                bestScore2 = score2;
                bestNode = newNode;
                bestRectIndex = i;
            }
        }

        if (bestRectIndex == -1)
            return;

        PlaceRect(bestNode);
        rects.erase(rects.begin() + bestRectIndex);
    }
}

void MaxRectsBinPack::PlaceRect(const Rect &node)
{
    size_t numRectanglesToProcess = freeRectangles.size();
    for(size_t i = 0; i < numRectanglesToProcess; ++i)
    {
        if (SplitFreeNode(freeRectangles[i], node))
        {
            freeRectangles.erase(freeRectangles.begin() + i);
            --i;
            --numRectanglesToProcess;
        }
    }

    PruneFreeList();

    usedRectangles.push_back(node);
    //dst.push_back(bestNode); ///\todo Refactor so that this compiles.
}

Rect MaxRectsBinPack::ScoreRect(const RectSize &rcSz, FreeRectChoiceHeuristic method, int &score1, int &score2) const
{
    Rect newNode;
    score1 = std::numeric_limits<int>::max();
    score2 = std::numeric_limits<int>::max();
    switch(method)
    {
    case RectBestShortSideFit:
        newNode = FindPositionForNewNodeBestShortSideFit(rcSz, score1, score2);
        break;
    case RectBottomLeftRule:
        newNode = FindPositionForNewNodeBottomLeft(rcSz, score1, score2);
        break;
    case RectContactPointRule:
        newNode = FindPositionForNewNodeContactPoint(rcSz, score1);
        score1 = -score1; // Reverse since we are minimizing, but for contact point score bigger is better.
        break;
    case RectBestLongSideFit:
        newNode = FindPositionForNewNodeBestLongSideFit(rcSz, score2, score1);
        break;
    case RectBestAreaFit:
        newNode = FindPositionForNewNodeBestAreaFit(rcSz, score1, score2);
        break;
    }

    // Cannot fit the current rectangle.
    if (newNode.height == 0)
    {
        score1 = std::numeric_limits<int>::max();
        score2 = std::numeric_limits<int>::max();
    }

    return newNode;
}

/// Computes the ratio of used surface area.
float MaxRectsBinPack::Occupancy() const
{
    unsigned long usedSurfaceArea = 0;
    for(size_t i = 0; i < usedRectangles.size(); ++i)
        usedSurfaceArea += usedRectangles[i].width * usedRectangles[i].height;

    return (float)usedSurfaceArea / (binWidth * binHeight);
}

Rect MaxRectsBinPack::FindPositionForNewNodeBottomLeft(const RectSize &rcSz, int &bestY, int &bestX) const
{
    Rect bestNode;
    // commenting by ouxh 2016/08/29，Rect内包含std::string类，memset引发内存暴增
    //memset(&bestNode, 0, sizeof(Rect));

    bestY = std::numeric_limits<int>::max();
    bestX = std::numeric_limits<int>::max();

    for(size_t i = 0; i < freeRectangles.size(); ++i)
    {
        // Try to place the rectangle in upright (non-flipped) orientation.
        if (freeRectangles[i].width >= rcSz.width && freeRectangles[i].height >= rcSz.height)
        {
            int topSideY = freeRectangles[i].y + rcSz.height;
            if (topSideY < bestY || (topSideY == bestY && freeRectangles[i].x < bestX))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.width;
                bestNode.height = rcSz.height;
                bestY = topSideY;
                bestX = freeRectangles[i].x;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = false;
            }
        }

        if (rcSz.rotate && freeRectangles[i].width >= rcSz.height && freeRectangles[i].height >= rcSz.width)
        {
            int topSideY = freeRectangles[i].y + rcSz.width;
            if (topSideY < bestY || (topSideY == bestY && freeRectangles[i].x < bestX))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.height;
                bestNode.height = rcSz.width;
                bestY = topSideY;
                bestX = freeRectangles[i].x;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = true;
            }
        }
    }
    return bestNode;
}

Rect MaxRectsBinPack::FindPositionForNewNodeBestShortSideFit(const RectSize &rcSz, int &bestShortSideFit, int &bestLongSideFit) const
{
    Rect bestNode;

    // commenting by ouxh 2016/08/29，Rect内包含std::string类，memset引发内存暴增
    //memset(&bestNode, 0, sizeof(Rect));

    bestShortSideFit = std::numeric_limits<int>::max();
    bestLongSideFit = std::numeric_limits<int>::max();

    for(size_t i = 0; i < freeRectangles.size(); ++i)
    {
        // Try to place the rectangle in upright (non-flipped) orientation.
        if (freeRectangles[i].width >= rcSz.width && freeRectangles[i].height >= rcSz.height)
        {
            int leftoverHoriz = abs(freeRectangles[i].width - rcSz.width);
            int leftoverVert = abs(freeRectangles[i].height - rcSz.height);
            int shortSideFit = min(leftoverHoriz, leftoverVert);
            int longSideFit = max(leftoverHoriz, leftoverVert);

            if (shortSideFit < bestShortSideFit || (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.width;
                bestNode.height = rcSz.height;
                bestShortSideFit = shortSideFit;
                bestLongSideFit = longSideFit;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = false;
            }
        }

        if (rcSz.rotate && freeRectangles[i].width >= rcSz.height && freeRectangles[i].height >= rcSz.width)
        {
            int flippedLeftoverHoriz = abs(freeRectangles[i].width - rcSz.height);
            int flippedLeftoverVert = abs(freeRectangles[i].height - rcSz.width);
            int flippedShortSideFit = min(flippedLeftoverHoriz, flippedLeftoverVert);
            int flippedLongSideFit = max(flippedLeftoverHoriz, flippedLeftoverVert);

            if (flippedShortSideFit < bestShortSideFit || (flippedShortSideFit == bestShortSideFit && flippedLongSideFit < bestLongSideFit))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.height;
                bestNode.height = rcSz.width;
                bestShortSideFit = flippedShortSideFit;
                bestLongSideFit = flippedLongSideFit;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = true;
            }
        }
    }
    return bestNode;
}

Rect MaxRectsBinPack::FindPositionForNewNodeBestLongSideFit(const RectSize &rcSz, int &bestShortSideFit, int &bestLongSideFit) const
{
    Rect bestNode;

    // commenting by ouxh 2016/08/29，Rect内包含std::string类，memset引发内存暴增
    //memset(&bestNode, 0, sizeof(Rect));

    bestShortSideFit = std::numeric_limits<int>::max();
    bestLongSideFit = std::numeric_limits<int>::max();

    for(size_t i = 0; i < freeRectangles.size(); ++i)
    {
        // Try to place the rectangle in upright (non-flipped) orientation.
        if (freeRectangles[i].width >= rcSz.width && freeRectangles[i].height >= rcSz.height)
        {
            int leftoverHoriz = abs(freeRectangles[i].width - rcSz.width);
            int leftoverVert = abs(freeRectangles[i].height - rcSz.height);
            int shortSideFit = min(leftoverHoriz, leftoverVert);
            int longSideFit = max(leftoverHoriz, leftoverVert);

            if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.width;
                bestNode.height = rcSz.height;
                bestShortSideFit = shortSideFit;
                bestLongSideFit = longSideFit;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = false;
            }
        }

        if (rcSz.rotate && freeRectangles[i].width >= rcSz.height && freeRectangles[i].height >= rcSz.width)
        {
            int leftoverHoriz = abs(freeRectangles[i].width - rcSz.height);
            int leftoverVert = abs(freeRectangles[i].height - rcSz.width);
            int shortSideFit = min(leftoverHoriz, leftoverVert);
            int longSideFit = max(leftoverHoriz, leftoverVert);

            if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.height;
                bestNode.height = rcSz.width;
                bestShortSideFit = shortSideFit;
                bestLongSideFit = longSideFit;


                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = true;
            }
        }
    }
    return bestNode;
}

Rect MaxRectsBinPack::FindPositionForNewNodeBestAreaFit(const RectSize &rcSz, int &bestAreaFit, int &bestShortSideFit) const
{
    Rect bestNode;

    // commenting by ouxh 2016/08/29，Rect内包含std::string类，memset引发内存暴增
    //memset(&bestNode, 0, sizeof(Rect));

    bestAreaFit = std::numeric_limits<int>::max();
    bestShortSideFit = std::numeric_limits<int>::max();

    for(size_t i = 0; i < freeRectangles.size(); ++i)
    {
        int areaFit = freeRectangles[i].width * freeRectangles[i].height - rcSz.width * rcSz.height;

        // Try to place the rectangle in upright (non-flipped) orientation.
        if (freeRectangles[i].width >= rcSz.width && freeRectangles[i].height >= rcSz.height)
        {
            int leftoverHoriz = abs(freeRectangles[i].width - rcSz.width);
            int leftoverVert = abs(freeRectangles[i].height - rcSz.height);
            int shortSideFit = min(leftoverHoriz, leftoverVert);

            if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.width;
                bestNode.height = rcSz.height;
                bestShortSideFit = shortSideFit;
                bestAreaFit = areaFit;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = false;
            }
        }

        if (rcSz.rotate && freeRectangles[i].width >= rcSz.height && freeRectangles[i].height >= rcSz.width)
        {
            int leftoverHoriz = abs(freeRectangles[i].width - rcSz.height);
            int leftoverVert = abs(freeRectangles[i].height - rcSz.width);
            int shortSideFit = min(leftoverHoriz, leftoverVert);

            if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.height;
                bestNode.height = rcSz.width;
                bestShortSideFit = shortSideFit;
                bestAreaFit = areaFit;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = true;
            }
        }
    }
    return bestNode;
}

/// Returns 0 if the two intervals i1 and i2 are disjoint, or the length of their overlap otherwise.
int CommonIntervalLength(int i1start, int i1end, int i2start, int i2end)
{
    if (i1end < i2start || i2end < i1start)
        return 0;
    return min(i1end, i2end) - max(i1start, i2start);
}

int MaxRectsBinPack::ContactPointScoreNode(int x, int y, int width, int height) const
{
    int score = 0;

    if (x == 0 || x + width == binWidth)
        score += height;
    if (y == 0 || y + height == binHeight)
        score += width;

    for(size_t i = 0; i < usedRectangles.size(); ++i)
    {
        if (usedRectangles[i].x == x + width || usedRectangles[i].x + usedRectangles[i].width == x)
            score += CommonIntervalLength(usedRectangles[i].y, usedRectangles[i].y + usedRectangles[i].height, y, y + height);
        if (usedRectangles[i].y == y + height || usedRectangles[i].y + usedRectangles[i].height == y)
            score += CommonIntervalLength(usedRectangles[i].x, usedRectangles[i].x + usedRectangles[i].width, x, x + width);
    }
    return score;
}

Rect MaxRectsBinPack::FindPositionForNewNodeContactPoint(const RectSize &rcSz, int &bestContactScore) const
{
    Rect bestNode;

    // commenting by ouxh 2016/08/29，Rect内包含std::string类，memset引发内存暴增
    //memset(&bestNode, 0, sizeof(Rect));

    bestContactScore = -1;

    for(size_t i = 0; i < freeRectangles.size(); ++i)
    {
        // Try to place the rectangle in upright (non-flipped) orientation.
        if (freeRectangles[i].width >= rcSz.width && freeRectangles[i].height >= rcSz.height)
        {
            int score = ContactPointScoreNode(freeRectangles[i].x, freeRectangles[i].y, rcSz.width, rcSz.height);
            if (score > bestContactScore)
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.width;
                bestNode.height = rcSz.height;
                bestContactScore = score;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = false;
            }
        }
        if (rcSz.rotate && freeRectangles[i].width >= rcSz.height && freeRectangles[i].height >= rcSz.width)
        {
            int score = ContactPointScoreNode(freeRectangles[i].x, freeRectangles[i].y, rcSz.height, rcSz.width);
            if (score > bestContactScore)
            {
                bestNode.x = freeRectangles[i].x;
                bestNode.y = freeRectangles[i].y;
                bestNode.width = rcSz.height;
                bestNode.height = rcSz.width;
                bestContactScore = score;

                // oxh add 2015-08-19
                bestNode.barcodeNo = rcSz.barcodeNo;
                bestNode.name = rcSz.name;
                bestNode.rotate = rcSz.rotate;
                bestNode.bRotated = true;
            }
        }
    }
    return bestNode;
}

bool MaxRectsBinPack::SplitFreeNode(Rect freeNode, const Rect &usedNode)
{
    // Test with SAT if the rectangles even intersect.
    if (usedNode.x >= freeNode.x + freeNode.width || usedNode.x + usedNode.width <= freeNode.x ||
            usedNode.y >= freeNode.y + freeNode.height || usedNode.y + usedNode.height <= freeNode.y)
        return false;

    if (usedNode.x < freeNode.x + freeNode.width && usedNode.x + usedNode.width > freeNode.x)
    {
        // New node at the top side of the used node.
        if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.height)
        {
            Rect newNode = freeNode;
            newNode.height = usedNode.y - newNode.y;
            freeRectangles.push_back(newNode);
        }

        // New node at the bottom side of the used node.
        if (usedNode.y + usedNode.height < freeNode.y + freeNode.height)
        {
            Rect newNode = freeNode;
            newNode.y = usedNode.y + usedNode.height;
            newNode.height = freeNode.y + freeNode.height - (usedNode.y + usedNode.height);
            freeRectangles.push_back(newNode);
        }
    }

    if (usedNode.y < freeNode.y + freeNode.height && usedNode.y + usedNode.height > freeNode.y)
    {
        // New node at the left side of the used node.
        if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.width)
        {
            Rect newNode = freeNode;
            newNode.width = usedNode.x - newNode.x;
            freeRectangles.push_back(newNode);
        }

        // New node at the right side of the used node.
        if (usedNode.x + usedNode.width < freeNode.x + freeNode.width)
        {
            Rect newNode = freeNode;
            newNode.x = usedNode.x + usedNode.width;
            newNode.width = freeNode.x + freeNode.width - (usedNode.x + usedNode.width);
            freeRectangles.push_back(newNode);
        }
    }

    return true;
}

void MaxRectsBinPack::PruneFreeList()
{
    /*
    ///  Would be nice to do something like this, to avoid a Theta(n^2) loop through each pair.
    ///  But unfortunately it doesn't quite cut it, since we also want to detect containment.
    ///  Perhaps there's another way to do this faster than Theta(n^2).

    if (freeRectangles.size() > 0)
        clb::sort::QuickSort(&freeRectangles[0], freeRectangles.size(), NodeSortCmp);

    for(size_t i = 0; i < freeRectangles.size()-1; ++i)
        if (freeRectangles[i].x == freeRectangles[i+1].x &&
            freeRectangles[i].y == freeRectangles[i+1].y &&
            freeRectangles[i].width == freeRectangles[i+1].width &&
            freeRectangles[i].height == freeRectangles[i+1].height)
        {
            freeRectangles.erase(freeRectangles.begin() + i);
            --i;
        }
    */

    /// Go through each pair and remove any rectangle that is redundant.
    for(size_t i = 0; i < freeRectangles.size(); ++i)
        for(size_t j = i+1; j < freeRectangles.size(); ++j)
        {
            if (IsContainedIn(freeRectangles[i], freeRectangles[j]))
            {
                freeRectangles.erase(freeRectangles.begin()+i);
                --i;
                break;
            }
            if (IsContainedIn(freeRectangles[j], freeRectangles[i]))
            {
                freeRectangles.erase(freeRectangles.begin()+j);
                --j;
            }
        }
}

}
