#include "drawinterlacing.h"

#include "base/math/renard.h"
#include "base/text/smartstring.h"
#include "chart/rendering/drawlabel.h"
#include "chart/rendering/orientedlabel.h"

namespace Vizzu::Draw
{

void DrawInterlacing::drawGeometries() const
{
	draw(true, false);
	draw(false, false);
}

void DrawInterlacing::drawTexts() const
{
	draw(true, true);
	draw(false, true);
}

void DrawInterlacing::draw(bool horizontal, bool text) const
{
	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;

	auto interlacingColor =
	    *rootStyle.plot.getAxis(axisIndex).interlacing.color;

	if (!text && interlacingColor.alpha <= 0.0) return;

	const auto &axis = plot->measureAxises.at(axisIndex);

	if (!axis.range.isReal()) return;

	auto enabled = axis.enabled.calculate<double>();

	auto step = axis.step.calculate();

	auto stepHigh = std::clamp(Math::Renard::R5().ceil(step),
	    axis.step.min(),
	    axis.step.max());
	auto stepLow = std::clamp(Math::Renard::R5().floor(step),
	    axis.step.min(),
	    axis.step.max());

	if (stepHigh == step) {
		draw(axis.enabled,
		    horizontal,
		    stepHigh,
		    enabled,
		    axis.range.size(),
		    text);
	}
	else if (stepLow == step) {
		draw(axis.enabled,
		    horizontal,
		    stepLow,
		    enabled,
		    axis.range.size(),
		    text);
	}
	else {
		auto highWeight =
		    Math::Range(stepLow, stepHigh).rescale(step) * enabled;

		auto lowWeight = (1.0 - highWeight) * enabled;

		draw(axis.enabled,
		    horizontal,
		    stepLow,
		    lowWeight,
		    axis.range.size(),
		    text);
		draw(axis.enabled,
		    horizontal,
		    stepHigh,
		    highWeight,
		    axis.range.size(),
		    text);
	}
}

void DrawInterlacing::draw(
    const ::Anim::Interpolated<bool> &axisEnabled,
    bool horizontal,
    double stepSize,
    double weight,
    double rangeSize,
    bool text) const
{
	const auto &enabled =
	    horizontal ? plot->guides.y : plot->guides.x;

	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;

	const auto &axisStyle = rootStyle.plot.getAxis(axisIndex);

	const auto &axis = plot->measureAxises.at(axisIndex);

	const auto origo = plot->measureAxises.origo();

	if (static_cast<double>(enabled.interlacings || enabled.axisSticks
	                        || enabled.labels)
	    > 0) {
		auto interlaceIntensity =
		    weight * static_cast<double>(enabled.interlacings);
		auto interlaceColor =
		    *axisStyle.interlacing.color * interlaceIntensity;

		auto tickIntensity =
		    weight * static_cast<double>(enabled.axisSticks);

		auto textAlpha = weight * static_cast<double>(enabled.labels);
		auto textColor = *axisStyle.label.color * textAlpha;

		if (rangeSize <= 0) return;

		auto stripWidth = stepSize / rangeSize;

		auto axisBottom = axis.origo() + stripWidth;

		auto iMin = axisBottom > 0 ? static_cast<int>(
		                std::floor(-axis.origo() / (2 * stripWidth)))
		                           : 0;

		if (stripWidth <= 0) return;
		auto interlaceCount = 0U;
		const auto maxInterlaceCount = 1000U;
		for (int i = iMin; ++interlaceCount <= maxInterlaceCount;
		     ++i) {
			auto bottom = axisBottom + i * 2 * stripWidth;
			if (bottom >= 1.0) break;
			auto clippedBottom = bottom;
			auto top = bottom + stripWidth;
			auto clipTop = top > 1.0;
			auto clipBottom = bottom < 0.0;
			auto topUnderflow = top <= 0.0;
			if (clipTop) top = 1.0;
			if (clipBottom) clippedBottom = 0.0;

			if (!topUnderflow) {
				Geom::Rect rect(Geom::Point{clippedBottom, 0.0},
				    Geom::Size{top - clippedBottom, 1.0});

				if (horizontal)
					rect = Geom::Rect{rect.pos.flip(),
					    {rect.size.flip()}};

				if (text) {
					canvas.setTextColor(textColor);
					canvas.setFont(Gfx::Font{axisStyle.label});

					if (!clipBottom) {
						auto value = (i * 2 + 1) * stepSize;
						auto tickPos =
						    rect.bottomLeft().comp(!horizontal)
						    + origo.comp(horizontal);

						if (textColor.alpha > 0)
							drawDataLabel(axisEnabled,
							    horizontal,
							    tickPos,
							    value,
							    axis.unit,
							    textColor);

						if (tickIntensity > 0)
							drawSticks(tickIntensity,
							    horizontal,
							    tickPos);
					}
					if (!clipTop) {
						auto value = (i * 2 + 2) * stepSize;
						auto tickPos =
						    rect.topRight().comp(!horizontal)
						    + origo.comp(horizontal);

						if (textColor.alpha > 0)
							drawDataLabel(axisEnabled,
							    horizontal,
							    tickPos,
							    value,
							    axis.unit,
							    textColor);

						if (tickIntensity > 0)
							drawSticks(tickIntensity,
							    horizontal,
							    tickPos);
					}
				}
				else {
					canvas.save();

					canvas.setLineColor(Gfx::Color::Transparent());
					canvas.setBrushColor(interlaceColor);

					painter.setPolygonToCircleFactor(0);
					painter.setPolygonStraightFactor(0);

					auto eventTarget =
					    Events::Targets::axisInterlacing(!horizontal);

					if (rootEvents.draw.plot.axis.interlacing->invoke(
					        Events::OnRectDrawEvent(*eventTarget,
					            {rect, true}))) {
						painter.drawPolygon(rect.points());
						renderedChart.emplace(Draw::Rect{rect, true},
						    std::move(eventTarget));
					}

					canvas.restore();
				}
			}
		}
	}
}

void DrawInterlacing::drawDataLabel(
    const ::Anim::Interpolated<bool> &axisEnabled,
    bool horizontal,
    const Geom::Point &tickPos,
    double value,
    const ::Anim::Interpolated<std::string> &unit,
    const Gfx::Color &textColor) const
{
	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;
	const auto &labelStyle = rootStyle.plot.getAxis(axisIndex).label;

	auto drawLabel = OrientedLabel{{ctx()}};
	labelStyle.position->visit(
	    [this,
	        &drawLabel,
	        &labelStyle,
	        &axisEnabled,
	        &tickPos,
	        &horizontal,
	        normal = Geom::Point::Ident(horizontal),
	        &unit,
	        &value,
	        &textColor](int index, const auto &position)
	    {
		    if (labelStyle.position->interpolates()
		        && !axisEnabled
		                .get(std::min<uint64_t>(axisEnabled.count - 1,
		                    index))
		                .value)
			    return;

		    Geom::Point refPos = tickPos;

		    switch (position.value) {
			    using Pos = Styles::AxisLabel::Position;
		    case Pos::min_edge:
			    refPos[horizontal ? 0 : 1] = 0.0;
			    break;
		    case Pos::max_edge:
			    refPos[horizontal ? 0 : 1] = 1.0;
			    break;
		    default: break;
		    }

		    auto under = labelStyle.position->interpolates()
		                   ? labelStyle.side
		                             ->get(std::min<uint64_t>(
		                                 labelStyle.side->count - 1,
		                                 index))
		                             .value
		                         == Styles::AxisLabel::Side::negative
		                   : labelStyle.side->factor<double>(
		                       Styles::AxisLabel::Side::negative);
		    unit.visit(
		        [this,
		            &drawLabel,
		            &unit,
		            &labelStyle,
		            &index,
		            &value,
		            posDir = coordSys
		                         .convertDirectionAt(
		                             {refPos, refPos + normal})
		                         .extend(1 - 2 * under),
		            &textColor,
		            &position,
		            &horizontal](int index2, const auto &wUnit)
		        {
			        if (labelStyle.position->interpolates()
			            && unit.interpolates() && index != index2)
				        return;
			        auto unitStr = wUnit.value;
			        auto str =
			            Text::SmartString::fromPhysicalValue(value,
			                *labelStyle.numberFormat,
			                static_cast<size_t>(
			                    *labelStyle.maxFractionDigits),
			                *labelStyle.numberScale,
			                unitStr);
			        drawLabel.draw(canvas,
			            str,
			            posDir,
			            labelStyle,
			            0,
			            textColor * position.weight * wUnit.weight,
			            *labelStyle.backgroundColor * wUnit.weight,
			            *rootEvents.draw.plot.axis.label,
			            Events::Targets::axisLabel({},
			                {},
			                str,
			                !horizontal));
		        });
	    });
}

void DrawInterlacing::drawSticks(double tickIntensity,
    bool horizontal,
    const Geom::Point &tickPos) const
{
	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;
	const auto &axisStyle = rootStyle.plot.getAxis(axisIndex);
	const auto &tickStyle = axisStyle.ticks;

	auto tickLength = tickStyle.length->get(
	    coordSys.getRect().size.getCoord(horizontal),
	    axisStyle.label.calculatedSize());

	if (tickStyle.color->isTransparent() || tickLength == 0
	    || *tickStyle.lineWidth == 0)
		return;

	auto tickColor = *tickStyle.color * tickIntensity;

	canvas.save();

	canvas.setLineColor(tickColor);
	canvas.setBrushColor(tickColor);

	auto direction =
	    horizontal ? Geom::Point::X(-1) : Geom::Point::Y(-1);

	auto tickLine =
	    coordSys.convertDirectionAt({tickPos, tickPos + direction});

	tickLine = tickLine.segment(0, tickLength);

	if (*tickStyle.lineWidth > 0)
		canvas.setLineWidth(*tickStyle.lineWidth);

	typedef Styles::Tick::Position Pos;
	tickLine = tickStyle.position->combine<Geom::Line>(
	    [&](int, const auto &position)
	    {
		    switch (position) {
		    default:
		    case Pos::outside: return tickLine;
		    case Pos::inside: return tickLine.segment(-1, 0);
		    case Pos::center: return tickLine.segment(-0.5, 0.5);
		    }
	    });

	auto eventTarget = Events::Targets::axisTick(!horizontal);

	if (rootEvents.draw.plot.axis.tick->invoke(
	        Events::OnLineDrawEvent(*eventTarget,
	            {tickLine, false}))) {
		canvas.line(tickLine);
		renderedChart.emplace(Draw::Line{tickLine, false},
		    std::move(eventTarget));
	}
	if (*tickStyle.lineWidth > 1) canvas.setLineWidth(0);

	canvas.restore();
}

}