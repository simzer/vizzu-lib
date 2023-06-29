#include "renderedchart.h"

#include <ranges>

using namespace Vizzu::Draw;

void RenderedChart::addElement(DrawingElement &&element)
{
	elements.push_back(std::move(element));
}

void Vizzu::Draw::RenderedChart::hintAddElementCount(size_t count)
{
	elements.reserve(elements.size() + count);
}

const DrawingElement &RenderedChart::findElement(const Geom::Point &point) const
{
	for (auto &element : std::ranges::reverse_view(elements))
		if (element.rect.contains(point))
			return element;
	return elements.front();
}