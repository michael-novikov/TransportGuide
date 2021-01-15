#include "map_builder.h"

#include "transport_manager_command.h"
#include "stop.h"
#include "svg.h"

#include <algorithm>
#include <utility>
#include <cstdlib>

using namespace std;
using namespace Svg;

const std::unordered_map<MapLayer, void (MapBuilder::*)(Svg::Document&) const> MapBuilder::build = {
  { MapLayer::BUS_LINES, &MapBuilder::BuildBusLines },
  { MapLayer::BUS_LABELS, &MapBuilder::BuildBusLabels },
  { MapLayer::STOP_POINTS, &MapBuilder::BuildStopPoints },
  { MapLayer::STOP_LABELS, &MapBuilder::BuildStopLabels },
};

MapBuilder::MapBuilder(RenderSettings render_settings,
                       const std::vector<Stop>& stops,
                       const std::map<std::string, size_t>& stop_idx,
                       const std::map<BusRoute::RouteNumber, BusRoute>& buses)
  : render_settings_(move(render_settings))
  , stop_idx_(stop_idx)
  , buses_(buses)
  , sorted_lon_(stops)
  , sorted_lat_(stops)
{
  sort(begin(sorted_lon_), end(sorted_lon_),
      [](const Stop& lhs, const Stop& rhs) {
        return lhs.StopCoordinates().longitude < rhs.StopCoordinates().longitude;
      });
  for (size_t i = 0; i < sorted_lon_.size(); ++i) {
    sorted_lon_idx_[sorted_lon_[i].Name()] = i;
  }

  sort(begin(sorted_lat_), end(sorted_lat_),
      [](const Stop& lhs, const Stop& rhs) {
        return lhs.StopCoordinates().latitude < rhs.StopCoordinates().latitude;
      });
  for (size_t i = 0; i < sorted_lat_.size(); ++i) {
    sorted_lat_idx_[sorted_lat_[i].Name()] = i;
  }

  if (sorted_lon_.size() > 1) {
    x_step = (render_settings_.width - 2 * render_settings_.padding) / (sorted_lon_.size() - 1);
  }

  if (sorted_lat_.size() > 1) {
    y_step = (render_settings_.height - 2 * render_settings_.padding) / (sorted_lat_.size() - 1);
  }

  doc_ = BuildMap();
}

Svg::Document MapBuilder::BuildMap() {
  Svg::Document doc;

  for (const auto& layer : render_settings_.layers) {
    (this->*build.at(layer))(doc);
  }

  return doc;
}

Svg::Point MapBuilder::MapStop(const std::string& stop_name) const {
  return {
    sorted_lon_idx_.at(stop_name) * x_step + render_settings_.padding,
    render_settings_.height - render_settings_.padding - sorted_lat_idx_.at(stop_name) * y_step,
  };
}

void MapBuilder::BuildBusLines(Svg::Document& doc) const {
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

    doc.Add(route_line);
    color_index = (color_index + 1) % render_settings_.color_palette.size();
  }
}

void MapBuilder::BuildBusLabels(Svg::Document& doc) const {
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
    doc.Add(
      create_bus_no_text(bus_no, first_stop)
      .SetFillColor(render_settings_.underlayer_color)
      .SetStrokeColor(render_settings_.underlayer_color)
      .SetStrokeWidth(render_settings_.underlayer_width)
      .SetStrokeLineCap("round")
      .SetStrokeLineJoin("round")
    );
    doc.Add(
      create_bus_no_text(bus_no, first_stop)
      .SetFillColor(stroke_color)
    );

    const auto last_stop = bus.Stops()[bus.Stops().size() / 2];
    if (!bus.IsRoundTrip() && first_stop != last_stop) {
      doc.Add(
        create_bus_no_text(bus_no, last_stop)
        .SetFillColor(render_settings_.underlayer_color)
        .SetStrokeColor(render_settings_.underlayer_color)
        .SetStrokeWidth(render_settings_.underlayer_width)
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round")
      );
      doc.Add(
        create_bus_no_text(bus_no, last_stop)
        .SetFillColor(stroke_color)
      );
    }

    route_no_color_index =
        (route_no_color_index + 1) % render_settings_.color_palette.size();
  }
}

void MapBuilder::BuildStopPoints(Svg::Document& doc) const {
  for (const auto& [stop_name, stop_id] : stop_idx_) {
    doc.Add(
      Circle{}
      .SetCenter(MapStop(stop_name))
      .SetRadius(render_settings_.stop_radius)
      .SetFillColor("white")
    );
  }
}

void MapBuilder::BuildStopLabels(Svg::Document& doc) const {
  for (const auto& [stop_name, stop_id] : stop_idx_) {
    const auto common =
      Text{}
      .SetPoint(MapStop(stop_name))
      .SetOffset(render_settings_.stop_label_offset)
      .SetFontSize(render_settings_.stop_label_font_size)
      .SetFontFamily("Verdana")
      .SetData(stop_name);

    doc.Add(
      Text{common}
      .SetFillColor(render_settings_.underlayer_color)
      .SetStrokeColor(render_settings_.underlayer_color)
      .SetStrokeWidth(render_settings_.underlayer_width)
      .SetStrokeLineCap("round")
      .SetStrokeLineJoin("round")
    );

    doc.Add(
      Text{common}
      .SetFillColor("black")
    );
  }
}
