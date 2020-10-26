#include "map_builder.h"

#include "transport_manager_command.h"
#include "stop.h"
#include "svg.h"

#include <algorithm>
#include <utility>
#include <cstdlib>

using namespace std;
using namespace Svg;

static const double EPS = 0.000001;

MapBuilder::MapBuilder(RenderSettings render_settings,
                       const std::vector<Stop>& stops,
                       const std::map<std::string, size_t>& stop_idx,
                       const std::map<BusRoute::RouteNumber, BusRoute>& buses)
  : render_settings_(move(render_settings))
  , stops_(stops)
  , stop_idx_(stop_idx)
  , buses_(buses)
{
  auto [min_lat_it, max_lat_it] = minmax_element(begin(stops), end(stops),
      [](const Stop& lhs, const Stop& rhs) {
        return lhs.StopCoordinates().latitude < rhs.StopCoordinates().latitude;
      });
  min_lat_ = min_lat_it->StopCoordinates().latitude;
  max_lat_ = max_lat_it->StopCoordinates().latitude;

  auto [min_lon_it, max_lon_it] = minmax_element(begin(stops), end(stops),
      [](const Stop& lhs, const Stop& rhs) {
        return lhs.StopCoordinates().longitude < rhs.StopCoordinates().longitude;
      });
  min_lon_ = min_lon_it->StopCoordinates().longitude;
  max_lon_ = max_lon_it->StopCoordinates().longitude;

  auto width_zoom_coef = [this]() {
    return (render_settings_.width - 2 * render_settings_.padding) / (max_lon_ - min_lon_);
  };
  auto height_zoom_coef = [this]() {
    return (render_settings_.height - 2 * render_settings_.padding) / (max_lat_ - min_lat_);
  };

  if ((abs(max_lon_ - min_lon_) < EPS) && (abs(max_lat_ - min_lat_) < EPS)) {
    zoom_coef_ = 0.0;
  } else if (abs(max_lon_ - min_lon_) < EPS) {
    zoom_coef_ = height_zoom_coef();
  } else if (abs(max_lat_ - min_lat_) < EPS) {
    zoom_coef_ = width_zoom_coef();
  } else {
    zoom_coef_ = min(width_zoom_coef(), height_zoom_coef());
  }

  BuildMap();
}

void MapBuilder::BuildMap() {
  for (const auto& layer : render_settings_.layers) {
    switch (layer) {
      case MapLayer::BUS_LINES:
        BuildBusLines();
        break;
      case MapLayer::BUS_LABELS:
        BuildBusLabels();
        break;
      case MapLayer::STOP_POINTS:
        BuildStopPoints();
        break;
      case MapLayer::STOP_LABELS:
        BuildStopLabels();
        break;
    }
  }
}

Svg::Point MapBuilder::MapStop(const std::string& stop_name) const {
  const auto &coordinates = stops_[stop_idx_.at(stop_name)].StopCoordinates();
  return {
    (coordinates.longitude - min_lon_) * zoom_coef_ + render_settings_.padding,
    (max_lat_ - coordinates.latitude) * zoom_coef_ + render_settings_.padding,
  };
}

void MapBuilder::BuildBusLines() {
  int color_index{0};
  for (const auto& [bus_no, bus] : buses_) {
    const Color& stroke_color = render_settings_.color_palette[color_index];

    auto route_line = Polyline{}
      .SetStrokeColor(stroke_color)
      .SetStrokeWidth(render_settings_.line_width)
      .SetStrokeLineCap("round")
      .SetStrokeLineJoin("round");

    for (const auto& stop_name : bus.Stops()) {
      route_line.AddPoint(MapStop(stop_name));
    }

    doc_.Add(route_line);
    color_index = (color_index + 1) % render_settings_.color_palette.size();
  }
}

void MapBuilder::BuildBusLabels() {
  int route_no_color_index{0};
  for (const auto& [bus_no, bus] : buses_) {
    const Color &stroke_color = render_settings_.color_palette[route_no_color_index];

    auto create_bus_no_text = [&](string bus_no, const string& stop_name) -> Text {
      return Text{}
          .SetPoint(MapStop(stop_name))
          .SetOffset(render_settings_.bus_label_offset)
          .SetFontSize(render_settings_.bus_label_font_size)
          .SetFontFamily("Verdana")
          .SetFontWeight("bold")
          .SetData(bus_no);
    };

    const auto& first_stop = bus.Stops().front();
    doc_.Add(
      create_bus_no_text(bus_no, first_stop)
      .SetFillColor(render_settings_.underlayer_color)
      .SetStrokeColor(render_settings_.underlayer_color)
      .SetStrokeWidth(render_settings_.underlayer_width)
      .SetStrokeLineCap("round")
      .SetStrokeLineJoin("round")
    );
    doc_.Add(
      create_bus_no_text(bus_no, first_stop)
      .SetFillColor(stroke_color)
    );

    const auto last_stop = bus.Stops()[bus.Stops().size() / 2];
    if (!bus.IsRoundTrip() && first_stop != last_stop) {
      doc_.Add(
        create_bus_no_text(bus_no, last_stop)
        .SetFillColor(render_settings_.underlayer_color)
        .SetStrokeColor(render_settings_.underlayer_color)
        .SetStrokeWidth(render_settings_.underlayer_width)
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round")
      );
      doc_.Add(
        create_bus_no_text(bus_no, last_stop)
        .SetFillColor(stroke_color)
      );
    }

    route_no_color_index =
        (route_no_color_index + 1) % render_settings_.color_palette.size();
  }
}

void MapBuilder::BuildStopPoints() {
  for (const auto& [stop_name, stop_id] : stop_idx_) {
    doc_.Add(
      Circle{}
      .SetCenter(MapStop(stop_name))
      .SetRadius(render_settings_.stop_radius)
      .SetFillColor("white")
    );
  }
}

void MapBuilder::BuildStopLabels() {
  for (const auto& [stop_name, stop_id] : stop_idx_) {
    const auto common =
      Text{}
      .SetPoint(MapStop(stop_name))
      .SetOffset(render_settings_.stop_label_offset)
      .SetFontSize(render_settings_.stop_label_font_size)
      .SetFontFamily("Verdana")
      .SetData(stop_name);

    doc_.Add(
      Text{common}
      .SetFillColor(render_settings_.underlayer_color)
      .SetStrokeColor(render_settings_.underlayer_color)
      .SetStrokeWidth(render_settings_.underlayer_width)
      .SetStrokeLineCap("round")
      .SetStrokeLineJoin("round")
    );

    doc_.Add(
      Text{common}
      .SetFillColor("black")
    );
  }
}
