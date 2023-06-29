#ifndef CHART_RENDERING_RENDEREDCHART_H
#define CHART_RENDERING_RENDEREDCHART_H

#include <vector>
#include <concepts>

#include "base/geom/affinetransform.h"
#include "base/geom/rect.h"
#include "base/util/eventdispatcher.h"

#include "chart/rendering/painter/coordinatesystem.h"

namespace Vizzu
{

namespace Draw
{

class DrawingElement {
public:
	DrawingElement(/*const Util::EventTarget &target*/)/* : target(target)*/
	{}

	Geom::Rect rect;
	Geom::AffineTransform transform;

private:
//	const Util::EventTarget &target;
};

class RenderedChart
{
public:
	RenderedChart() = default;
	RenderedChart(const CoordinateSystem &coordinateSystem)
		: coordinateSystem(coordinateSystem) {}

	void addElement(DrawingElement &&element);
	void hintAddElementCount(size_t count);

	const DrawingElement &findElement(const Geom::Point &point) const;

private:
	CoordinateSystem coordinateSystem;
	std::vector<DrawingElement> elements;

};

}
}

#endif
