#ifndef CHART_GENERATOR_MARKER_H
#define CHART_GENERATOR_MARKER_H

#include "base/anim/interpolated.h"
#include "base/geom/circle.h"
#include "base/geom/point.h"
#include "base/gfx/color.h"
#include "base/math/fuzzybool.h"
#include "chart/main/style.h"
#include "chart/options/options.h"
#include "data/datacube/datacube.h"

#include "colorbuilder.h"

namespace Vizzu
{
namespace Gen
{

class ChannelsStats;

class Marker
{
public:
	Marker(const Options &options,
	    const Styles::Chart &style,
	    const Data::DataCube &data,
	    const Data::DataTable &table,
	    ChannelsStats &stats,
	    const Data::MultiDim::MultiIndex &index,
	    size_t idx);

	Data::MultiDim::MultiIndex index;
	Gfx::Color color;
	ColorBuilder colorBuilder;
	Geom::Point position;
	Geom::Point size;
	Geom::Point spacing;
	double sizeFactor;
	Math::FuzzyBool enabled;
	Math::FuzzyBool selected;
	Data::CellInfo cellInfo;

	struct Label
	{
		constexpr static uint64_t INVALID_MEASURE = static_cast<uint64_t>(-1);
		double value;
		Data::ColumnIndex measureId;
		std::string unit;
		std::string indexStr;
		Label() : value(0.0), measureId(INVALID_MEASURE) {}
		Label(const Data::MultiDim::SubSliceIndex &index,
		    const Data::DataCube &data,
		    const Data::DataTable &table);
		Label(double value,
		    const Data::SeriesIndex &measure,
		    const Data::MultiDim::SubSliceIndex &index,
		    const Data::DataCube &data,
		    const Data::DataTable &table);
		bool operator==(const Label &other) const;
		bool hasValue() const { return measureId != INVALID_MEASURE; }
		std::string getIndexString(
		    const Data::MultiDim::SubSliceIndex &index,
		    const Data::DataCube &data,
		    const Data::DataTable &table) const;
	};

	::Anim::Interpolated<Label> label;

	struct Id
	{
		uint64_t seriesId;
		Data::MultiDim::SubSliceIndex itemSliceIndex;
		uint64_t itemId;
		Id() {}
		bool operator==(const Id &other) const = default;
		Id(const Data::DataCube &,
		    const Channel::DimensionIndices &dimensionIds,
		    const Data::MultiDim::MultiIndex &);
	};

	::Anim::Interpolated<Id> mainId;
	Id subId;
	Id sizeId;
	Id stackId;

	uint64_t idx;
	::Anim::Interpolated<uint64_t> prevMainMarkerIdx;
	::Anim::Interpolated<uint64_t> nextMainMarkerIdx;
	::Anim::Interpolated<uint64_t> nextSubMarkerIdx;

	void setNextMarker(uint64_t itemId,
	    Marker *marker,
	    bool horizontal,
	    bool main);
	void resetSize(bool horizontal);

	Geom::Rect toRectangle() const;
	void fromRectangle(const Geom::Rect &rect);

	Math::Range<double> getSizeBy(bool horizontal) const;
	void setSizeBy(bool horizontal, const Math::Range<double> range);

	void setIdOffset(size_t offset);
	std::string toJson(const Data::DataTable &table) const;

private:
	double getValueForChannel(const Channels &channels,
	    ChannelId type,
	    const Data::DataCube &data,
	    ChannelsStats &stats,
	    const Channel *subChannel = nullptr,
	    bool inhibitStack = false) const;
};

}
}

#endif
