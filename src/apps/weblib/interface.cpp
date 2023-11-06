#include "interface.h"

#include <span>

#include "base/conv/auto_json.h"
#include "base/io/log.h"

#include "canvas.h"
#include "interfacejs.h"
#include "jscriptcanvas.h"
#include "jsfunctionwrapper.h"

namespace Vizzu
{

template <class T, class Deleter>
std::unique_ptr<T, Deleter> create_unique_ptr(T *&&ptr,
    Deleter &&deleter)
{
	return {ptr, std::forward<Deleter>(deleter)};
}

Interface &Interface::getInstance()
{
	static Interface instance;
	return instance;
};

Interface::Interface()
{
	IO::Log::setEnabled(false);
	IO::Log::setTimestamp(false);
}

const char *Interface::version()
{
	static const std::string versionStr{Main::version};
	return versionStr.c_str();
}

std::shared_ptr<Vizzu::Chart> Interface::getChart(
    ObjectRegistry::Handle chart)
{
	auto &&widget = objects.get<UI::ChartWidget>(chart);
	auto &chartRef = widget->getChart();
	return {std::move(widget), &chartRef};
}

ObjectRegistry::Handle Interface::storeChart(
    ObjectRegistry::Handle chart)
{
	auto &&chartPtr = getChart(chart);
	return objects.reg(
	    std::make_shared<Snapshot>(chartPtr->getOptions(),
	        chartPtr->getStyles()));
}

void Interface::restoreChart(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle snapshot)
{
	auto &&snapshotPtr = objects.get<Snapshot>(snapshot);
	auto &&chartPtr = getChart(chart);
	chartPtr->setOptions(snapshotPtr->options);
	chartPtr->setStyles(snapshotPtr->styles);
}

ObjectRegistry::Handle Interface::storeAnim(
    ObjectRegistry::Handle chart)
{
	auto &&chartPtr = getChart(chart);
	return objects.reg(
	    std::make_shared<Animation>(chartPtr->getAnimation(),
	        Snapshot(chartPtr->getOptions(), chartPtr->getStyles())));
}

void Interface::restoreAnim(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle animPtr)
{
	auto &&anim = objects.get<Animation>(animPtr);
	auto &&chartPtr = getChart(chart);
	chartPtr->setAnimation(anim->animation);
	chartPtr->setOptions(anim->snapshot.options);
	chartPtr->setStyles(anim->snapshot.styles);
}

void Interface::freeObj(ObjectRegistry::Handle ptr)
{
	objects.unreg(ptr);
}

const char *Interface::getStyleList()
{
	static const std::string res =
	    Conv::toJSON(Styles::Sheet::paramList());
	return res.c_str();
}

const char *Interface::getStyleValue(ObjectRegistry::Handle chart,
    const char *path,
    bool computed)
{
	auto &&chartPtr = getChart(chart);
	thread_local std::string res;
	auto &styles = computed ? chartPtr->getComputedStyles()
	                        : chartPtr->getStyles();
	res = Styles::Sheet::getParam(styles, path);
	return res.c_str();
}

void Interface::setStyleValue(ObjectRegistry::Handle chart,
    const char *path,
    const char *value)
{
	getChart(chart)->getStylesheet().setParams(path, value);
}

const char *Interface::getChartParamList()
{
	static const std::string res =
	    Conv::toJSON(Gen::Config::listParams());
	return res.c_str();
}

const char *Interface::getChartValue(ObjectRegistry::Handle chart,
    const char *path)
{
	thread_local std::string res;
	res = getChart(chart)->getConfig().getParam(path);
	return res.c_str();
}

void Interface::setChartValue(ObjectRegistry::Handle chart,
    const char *path,
    const char *value)
{
	getChart(chart)->getConfig().setParam(path, value);
}

void Interface::relToCanvasCoords(ObjectRegistry::Handle chart,
    double rx,
    double ry,
    double &x,
    double &y)
{
	auto to = getChart(chart)->getCoordSystem().convert({rx, ry});
	x = to.x;
	y = to.y;
}

void Interface::canvasToRelCoords(ObjectRegistry::Handle chart,
    double x,
    double y,
    double &rx,
    double &ry)
{
	auto to = getChart(chart)->getCoordSystem().getOriginal({x, y});
	rx = to.x;
	ry = to.y;
}

void Interface::setChartFilter(ObjectRegistry::Handle chart,
    JsFunctionWrapper<bool, const Data::RowWrapper &> &&filter)
{
	const auto hash = filter.hash();
	getChart(chart)->getConfig().setFilter(
	    Data::Filter::Function{std::move(filter)},
	    hash);
}

std::variant<const char *, double> Interface::getRecordValue(
    const Data::RowWrapper &record,
    const char *column)
{
	auto cell = record[column];
	if (cell.isDimension()) return cell.dimensionValue();

	return *cell;
}

void Interface::addEventListener(ObjectRegistry::Handle chart,
    const char *event,
    void (*callback)(ObjectRegistry::Handle, const char *))
{
	auto &&chartPtr = getChart(chart);
	if (auto &&ev = chartPtr->getEventDispatcher().getEvent(event)) {
		ev->attach(std::hash<decltype(callback)>{}(callback),
		    [this, callback](Util::EventDispatcher::Params &params)
		    {
			    auto &&jsonStrIn = params.toJSON();

			    callback(
			        create_unique_ptr(
			            objects.reg<Util::EventDispatcher::Params>(
			                {std::shared_ptr<void>{}, &params}),
			            [this](const void *handle)
			            {
				            objects.unreg(handle);
			            })
			            .get(),
			        jsonStrIn.c_str());
		    });
	}
}

void Interface::removeEventListener(ObjectRegistry::Handle chart,
    const char *event,
    void (*callback)(ObjectRegistry::Handle, const char *))
{
	auto &&chartPtr = getChart(chart);
	if (auto &&ev = chartPtr->getEventDispatcher().getEvent(event)) {
		ev->detach(std::hash<decltype(callback)>{}(callback));
	}
}

void Interface::preventDefaultEvent(ObjectRegistry::Handle obj)
{
	objects.get<Util::EventDispatcher::Params>(obj)->preventDefault =
	    true;
}

void Interface::animate(ObjectRegistry::Handle chart,
    void (*callback)(bool))
{
	getChart(chart)->animate(callback);
}

void Interface::setKeyframe(ObjectRegistry::Handle chart)
{
	getChart(chart)->setKeyframe();
}

void Interface::animControl(ObjectRegistry::Handle chart,
    const char *command,
    const char *param)
{
	auto &&chartPtr = getChart(chart);
	auto &ctrl = chartPtr->getAnimControl();
	const std::string cmd(command);
	if (cmd == "seek")
		ctrl.seek(param);
	else if (cmd == "setSpeed")
		ctrl.setSpeed(std::stod(param));
	else if (cmd == "pause")
		ctrl.pause();
	else if (cmd == "play")
		ctrl.play();
	else if (cmd == "stop")
		ctrl.stop();
	else if (cmd == "cancel")
		ctrl.cancel();
	else if (cmd == "reverse")
		ctrl.reverse();
	else
		throw std::logic_error("invalid animation command");
}

void Interface::setAnimValue(ObjectRegistry::Handle chart,
    const char *path,
    const char *value)
{
	getChart(chart)->getAnimOptions().set(path, value);
}

void Interface::addDimension(ObjectRegistry::Handle chart,
    const char *name,
    const char **categories,
    int count)
{
	if (categories) {
		getChart(chart)->getTable().addColumn(name,
		    {categories, static_cast<size_t>(count)});
	}
}

void Interface::addMeasure(ObjectRegistry::Handle chart,
    const char *name,
    const char *unit,
    double *values,
    int count)
{
	getChart(chart)->getTable().addColumn(name,
	    unit,
	    {values, static_cast<size_t>(count)});
}

void Interface::addRecord(ObjectRegistry::Handle chart,
    const char **cells,
    int count)
{
	getChart(chart)->getTable().pushRow(
	    {cells, static_cast<size_t>(count)});
}

const char *Interface::dataMetaInfo(ObjectRegistry::Handle chart)
{
	thread_local std::string res;
	res = Conv::toJSON(getChart(chart)->getTable().getInfos());
	return res.c_str();
}

ObjectRegistry::Handle Interface::createChart()
{
	auto &&widget = std::make_shared<UI::ChartWidget>();

	widget->doSetCursor =
	    [&](const std::shared_ptr<Gfx::ICanvas> &target,
	        GUI::Cursor cursor)
	{
		::canvas_setCursor(
		    std::static_pointer_cast<Vizzu::Main::JScriptCanvas>(
		        target)
		        .get(),
		    toCSS(cursor));
	};
	widget->openUrl = [&](const std::string &url)
	{
		::openUrl(url.c_str());
	};

	return objects.reg(std::move(widget));
}

ObjectRegistry::Handle Interface::createCanvas()
{
	return objects.reg(
	    std::make_shared<Vizzu::Main::JScriptCanvas>());
}

void Interface::setLogging(bool enable)
{
	IO::Log::setEnabled(enable);
}

void Interface::update(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle canvas,
    double width,
    double height,
    double timeInMSecs,
    RenderControl renderControl)
{
	auto &&widget = objects.get<UI::ChartWidget>(chart);

	std::chrono::duration<double, std::milli> milliSecs(timeInMSecs);

	auto nanoSecs =
	    std::chrono::duration_cast<std::chrono::nanoseconds>(
	        milliSecs);

	::Anim::TimePoint time(nanoSecs);

	widget->getChart().getAnimControl().update(time);

	const Geom::Size size{width, height};

	auto &&canvasPtr =
	    objects.get<Vizzu::Main::JScriptCanvas>(canvas);

	const bool renderNeeded = widget->needsUpdate(canvasPtr)
	                       || widget->getSize(canvasPtr) != size;

	if ((renderControl == allow && renderNeeded)
	    || renderControl == force) {
		canvasPtr->frameBegin();
		widget->onUpdateSize(canvasPtr, size);
		widget->onDraw(canvasPtr);
		canvasPtr->frameEnd();
	}
}

void Interface::pointerDown(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle canvas,
    int pointerId,
    double x,
    double y)
{
	objects.get<UI::ChartWidget>(chart)->onPointerDown(
	    objects.get<Vizzu::Main::JScriptCanvas>(canvas),
	    GUI::PointerEvent(pointerId, Geom::Point{x, y}));
}

void Interface::pointerUp(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle canvas,
    int pointerId,
    double x,
    double y)
{
	objects.get<UI::ChartWidget>(chart)->onPointerUp(
	    objects.get<Vizzu::Main::JScriptCanvas>(canvas),
	    GUI::PointerEvent(pointerId, Geom::Point{x, y}));
}

void Interface::pointerLeave(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle canvas,
    int pointerId)
{
	objects.get<UI::ChartWidget>(chart)->onPointerLeave(
	    objects.get<Vizzu::Main::JScriptCanvas>(canvas),
	    GUI::PointerEvent(pointerId, Geom::Point::Invalid()));
}

void Interface::wheel(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle canvas,
    double delta)
{
	objects.get<UI::ChartWidget>(chart)->onWheel(
	    objects.get<Vizzu::Main::JScriptCanvas>(canvas),
	    delta);
}

void Interface::pointerMove(ObjectRegistry::Handle chart,
    ObjectRegistry::Handle canvas,
    int pointerId,
    double x,
    double y)
{
	objects.get<UI::ChartWidget>(chart)->onPointerMove(
	    objects.get<Vizzu::Main::JScriptCanvas>(canvas),
	    GUI::PointerEvent(pointerId, Geom::Point{x, y}));
}

}