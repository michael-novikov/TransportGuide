#include "map_builder.h"

#include "stop.h"
#include "svg.h"
#include "transport_manager_command.h"

#include "projector_interface.h"
#include "scanline_compressed_projection.h"
#include "scanline_projection.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <utility>

using namespace std;
using namespace Svg;

static vector<Coordinates> RecomputeBasedOnReferencePoint(
    std::vector<Stop> stops,
    const std::map<BusRoute::RouteNumber, BusRoute> &buses) {
  std::map<std::string, Coordinates *> stop_ptr;
  for (auto &stop : stops) {
    stop_ptr[stop.Name()] = &stop.StopCoordinates();
  }

  set<Coordinates> reference_points;

  // 1. endpoints
  for (const auto &[bus_name, bus] : buses) {
    {
      auto endpoints = bus.Endpoints();
      reference_points.insert(*stop_ptr.at(endpoints.first));
      if (endpoints.second) {
        reference_points.insert(*stop_ptr.at(endpoints.second.value()));
      }
    }
  }

  // 2. stops crossed more than twice by any bus
  for (const auto &stop : stops) {
    for (const auto &[bus_name, bus] : buses) {
      int count_stop = count_if(
          begin(bus.Stops()), end(bus.Stops()),
          [&stop](const auto &bus_stop) { return bus_stop == stop.Name(); });
      if (count_stop > 2) {
        reference_points.insert(stop.StopCoordinates());
      }
    }
  }

  // 3. stops crossed by more than one bus
  for (const auto &stop : stops) {
    int count_stop = count_if(begin(buses), end(buses), [&stop](const auto &p) {
      return p.second.ContainsStop(stop.Name());
    });
    if (count_stop > 1) {
      reference_points.insert(stop.StopCoordinates());
    }
  }

  for (const auto &[bus_name, bus] : buses) {
    size_t i{0};
    auto stop_count = bus.Stops().size();
    while (i < stop_count) {
      size_t j{i + 1};

      for (; j < stop_count; ++j) {
        if (reference_points.count(*stop_ptr.at(bus.Stop(j)))) {
          break;
        }
      }

      if (j == stop_count) {
        break;
      }

      const auto &stop_i = *stop_ptr.at(bus.Stop(i));
      const auto &stop_j = *stop_ptr.at(bus.Stop(j));

      double lon_step = (stop_j.longitude - stop_i.longitude) / (j - i);
      double lat_step = (stop_j.latitude - stop_i.latitude) / (j - i);

      for (size_t k = i + 1; k < j; ++k) {
        auto &stop_k = *stop_ptr.at(bus.Stop(k));
        stop_k.longitude = stop_i.longitude + lon_step * (k - i);
        stop_k.latitude = stop_i.latitude + lat_step * (k - i);
      }

      i = j;
    }
  }

  vector<Coordinates> points;
  transform(begin(stops), end(stops), back_inserter(points),
            [](const Stop &stop) { return stop.StopCoordinates(); });
  return points;
}

static map<string, Svg::Point>
ComputeStopsCoords(RenderSettings render_settings,
                   const std::vector<Stop> &stops,
                   const std::map<std::string, size_t> &stop_idx,
                   const std::map<BusRoute::RouteNumber, BusRoute> &buses) {
  const auto points = RecomputeBasedOnReferencePoint(stops, buses);
  const double max_width = render_settings.width;
  const double max_height = render_settings.height;
  const double padding = render_settings.padding;

  const unique_ptr<Projector> projector =
      make_unique<ScanlineCompressedProjector>(points, stop_idx, buses,
                                               max_width, max_height, padding);

  map<string, Svg::Point> stops_coords;
  for (size_t i = 0; i < stops.size(); ++i) {
    stops_coords[stops[i].Name()] = projector->project(points[i]);
  }
  return stops_coords;
}

MapBuilder::MapBuilder(RenderSettings render_settings,
                       const std::vector<Stop> &stops,
                       const std::map<std::string, size_t> &stop_idx,
                       const std::map<BusRoute::RouteNumber, BusRoute> &buses)
    : render_settings_(move(render_settings)), stop_idx_(stop_idx),
      buses_(buses), stop_coordinates_(ComputeStopsCoords(
                         render_settings_, stops, stop_idx_, buses)),
      doc_(BuildMap()) {}

Svg::Document MapBuilder::BuildMap() {
  Svg::Document doc;

  for (const auto &layer : render_settings_.layers) {
    (this->*build.at(layer))(doc);
  }

  return doc;
}

Svg::Point MapBuilder::MapStop(const std::string &stop_name) const {
  return stop_coordinates_.at(stop_name);
}

void MapBuilder::BuildBusLines(Svg::Document &doc) const {
  int color_index{0};
  for (const auto &[bus_no, bus] : buses_) {
    const Color &stroke_color = render_settings_.color_palette[color_index];

    auto route_line = Polyline{}
                          .SetStrokeColor(stroke_color)
                          .SetStrokeWidth(render_settings_.line_width)
                          .SetStrokeLineCap("round")
                          .SetStrokeLineJoin("round");

    for (const auto &stop_name : bus.Stops()) {
      route_line.AddPoint(MapStop(stop_name));
    }

    doc.Add(route_line);
    color_index = (color_index + 1) % render_settings_.color_palette.size();
  }
}

void MapBuilder::BuildBusLabels(Svg::Document &doc) const {
  int route_no_color_index{0};
  for (const auto &[bus_no, bus] : buses_) {
    const Color &stroke_color =
        render_settings_.color_palette[route_no_color_index];

    auto create_bus_no_text = [&](string bus_no,
                                  const string &stop_name) -> Text {
      return Text{}
          .SetPoint(MapStop(stop_name))
          .SetOffset(render_settings_.bus_label_offset)
          .SetFontSize(render_settings_.bus_label_font_size)
          .SetFontFamily("Verdana")
          .SetFontWeight("bold")
          .SetData(bus_no);
    };

    const auto &first_stop = bus.Stops().front();
    doc.Add(create_bus_no_text(bus_no, first_stop)
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round"));
    doc.Add(create_bus_no_text(bus_no, first_stop).SetFillColor(stroke_color));

    const auto last_stop = bus.Stops()[bus.Stops().size() / 2];
    if (!bus.IsRoundTrip() && first_stop != last_stop) {
      doc.Add(create_bus_no_text(bus_no, last_stop)
                  .SetFillColor(render_settings_.underlayer_color)
                  .SetStrokeColor(render_settings_.underlayer_color)
                  .SetStrokeWidth(render_settings_.underlayer_width)
                  .SetStrokeLineCap("round")
                  .SetStrokeLineJoin("round"));
      doc.Add(create_bus_no_text(bus_no, last_stop).SetFillColor(stroke_color));
    }

    route_no_color_index =
        (route_no_color_index + 1) % render_settings_.color_palette.size();
  }
}

void MapBuilder::BuildStopPoints(Svg::Document &doc) const {
  for (const auto &[stop_name, stop_id] : stop_idx_) {
    doc.Add(Circle{}
                .SetCenter(MapStop(stop_name))
                .SetRadius(render_settings_.stop_radius)
                .SetFillColor("white"));
  }
}

void MapBuilder::BuildStopLabels(Svg::Document &doc) const {
  for (const auto &[stop_name, stop_id] : stop_idx_) {
    const auto common = Text{}
                            .SetPoint(MapStop(stop_name))
                            .SetOffset(render_settings_.stop_label_offset)
                            .SetFontSize(render_settings_.stop_label_font_size)
                            .SetFontFamily("Verdana")
                            .SetData(stop_name);

    doc.Add(Text{common}
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round"));

    doc.Add(Text{common}.SetFillColor("black"));
  }
}

const std::unordered_map<MapLayer, void (MapBuilder::*)(Svg::Document &) const>
    MapBuilder::build = {
        {MapLayer::BUS_LINES, &MapBuilder::BuildBusLines},
        {MapLayer::BUS_LABELS, &MapBuilder::BuildBusLabels},
        {MapLayer::STOP_POINTS, &MapBuilder::BuildStopPoints},
        {MapLayer::STOP_LABELS, &MapBuilder::BuildStopLabels},
};
