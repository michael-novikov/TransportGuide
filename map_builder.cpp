#include "map_builder.h"

#include "router.pb.h"
#include "stop.h"
#include "svg.h"
#include "transport_manager_command.h"

#include "scanline_compressed_projection.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <set>
#include <utility>

namespace TransportGuide {

using namespace std;
using namespace Svg;

static unordered_map<string, Point> RecomputeBasedOnReferencePoint(const StopDict& stop_dict, const BusDict& bus_dict) {
  set<Point> reference_points;

  // 1. endpoints
  for (const auto &[bus_name, bus] : bus_dict) {
    auto endpoints = Endpoints(*bus);
    reference_points.insert(stop_dict.at(endpoints.first)->coordinates());
    if (endpoints.second) {
      reference_points.insert(stop_dict.at(endpoints.second.value())->coordinates());
    }
  }

  // 2. stops crossed more than twice by any bus
  for (const auto& [stop_name, stop] : stop_dict) {
    for (const auto &[bus_name, bus] : bus_dict) {
      auto count_stop = count_if(
        begin(bus->stops()), end(bus->stops()),
        [name = stop_name](const auto &bus_stop) { return bus_stop == name; }
      );
      if (count_stop > 2) {
        reference_points.insert(stop->coordinates());
      }
    }
  }

  // 3. stops crossed by more than one bus
  for (const auto& [stop_name, stop] : stop_dict) {
    if (stop->buses_size() > 1) {
      reference_points.insert(stop->coordinates());
    }
  }

  unordered_map<string, Point> point_dict;
  for (const auto& [stop_name, stop] : stop_dict) {
    point_dict[stop_name].CopyFrom(stop->coordinates());
  }

  for (const auto &[bus_name, bus] : bus_dict) {
    size_t i{0};
    auto stop_count = bus->stops_size();
    while (i < stop_count) {
      size_t j{i + 1};

      for (; j < stop_count; ++j) {
        if (reference_points.count(point_dict.at(bus->stops(j)))) {
          break;
        }
      }

      if (j == stop_count) {
        break;
      }

      const auto &stop_i = point_dict.at(bus->stops(i));
      const auto &stop_j = point_dict.at(bus->stops(j));

      double lon_step = static_cast<double>(stop_j.longitude() - stop_i.longitude()) / (j - i);
      double lat_step = static_cast<double>(stop_j.latitude() - stop_i.latitude()) / (j - i);

      for (size_t k = i + 1; k < j; ++k) {
        auto& stop_k = point_dict.at(bus->stops(k));
        stop_k.set_longitude(stop_i.longitude() + lon_step * (k - i));
        stop_k.set_latitude(stop_i.latitude() + lat_step * (k - i));
      }

      i = j;
    }
  }

  return point_dict;
}

std::map<std::string, SvgPoint> MapBuilder::ComputeStopsCoords(const RenderSettings& render_settings,
                                                               const StopDict& stop_dict,
                                                               const BusDict& buses) {
  auto point_dict = RecomputeBasedOnReferencePoint(stop_dict, buses);

  ScanlineCompressedProjector projector(
    point_dict,
    buses,
    render_settings.width(),
    render_settings.height(),
    render_settings.padding()
  );

  map<string, SvgPoint> stops_coords;
  for (const auto& [name, point] : point_dict) {
    stops_coords[name].CopyFrom(projector.project(point));
  }
  return stops_coords;
}

MapBuilder::MapBuilder(const RenderSettings& render_settings,
                       const StopDict& stop_dict,
                       const BusDict& bus_dict,
                       std::map<std::string, SvgPoint> coordinates_redefined,
                       std::string map_string)
    : render_settings_(render_settings)
    , stop_dict_(stop_dict)
    , bus_dict_(bus_dict)
    , stop_coordinates_(move(coordinates_redefined))
    , map_base_(map_string)
{
  int color_idx{0};
  for (const auto &[bus_no, bus] : bus_dict_) {
    route_color[bus_no] = render_settings_.color_palette(color_idx);
    color_idx = (color_idx + 1) % render_settings_.color_palette_size();
  }

  map_ = BuildMap();
  map_base_ = map_.BodyToString();
}

MapBuilder::MapBuilder(const RenderSettings& render_settings,
                       const StopDict& stop_dict,
                       const BusDict& bus_dict)
    : render_settings_(render_settings)
    , stop_dict_(stop_dict)
    , bus_dict_(bus_dict)
    , stop_coordinates_(ComputeStopsCoords(render_settings_, stop_dict_, bus_dict_))
{
  int color_idx{0};
  for (const auto &[bus_no, bus] : bus_dict_) {
    route_color[bus_no] = render_settings_.color_palette(color_idx);
    color_idx = (color_idx + 1) % render_settings_.color_palette_size();
  }

  map_ = BuildMap();
  map_base_ = map_.BodyToString();
}

Svg::Document MapBuilder::BuildMap() const {
  Svg::Document doc;

  for (const auto &layer : render_settings_.layers()) {
    (this->*build.at(layer))(doc);
  }

  return doc;
}

void MapBuilder::BuildTranslucentRoute(Svg::Document &doc) const {
  SvgPoint base_point;
  base_point.set_x(-render_settings_.outer_margin());
  base_point.set_y(-render_settings_.outer_margin());

  doc.Add(
    Rectangle{}
      .SetBasePoint(base_point)
      .SetWidth(render_settings_.width() + 2 * render_settings_.outer_margin())
      .SetHeight(render_settings_.height() + 2 * render_settings_.outer_margin())
      .SetFillColor(render_settings_.underlayer_color())
  );
}

Svg::Document MapBuilder::BuildRoute(const RouteDescription::Route &route) const {
  //auto res = BuildMap();
  Svg::Document res;
  BuildTranslucentRoute(res);

  for (const auto &layer : render_settings_.layers()) {
    (this->*build_route.at(layer))(res, route);
  }

  return res;
}

SvgPoint MapBuilder::MapStop(const std::string &stop_name) const {
  return stop_coordinates_.at(stop_name);
}

void MapBuilder::BuildBusLines(Svg::Document &doc,
    const std::vector<std::pair<std::string, std::vector<std::string>>> &lines) const {
  for (const auto &line : lines) {
    auto route_line = Polyline{}
                          .SetStrokeColor(route_color.at(line.first))
                          .SetStrokeWidth(render_settings_.line_width())
                          .SetStrokeLineCap("round")
                          .SetStrokeLineJoin("round");

    for (const auto &stop_name : line.second) {
      route_line.AddPoint(MapStop(stop_name));
    }
    doc.Add(route_line);
  }
}

void MapBuilder::BuildBusLines(Svg::Document &doc) const {
  vector<pair<string, vector<string>>> full_routes;
  for (const auto &[bus_no, bus] : bus_dict_) {
    full_routes.emplace_back(bus_no, vector(begin(bus->stops()), end(bus->stops())));
  }
  BuildBusLines(doc, full_routes);
}

void MapBuilder::BuildBusLinesOnRoute(Svg::Document &doc, const RouteDescription::Route &route) const {
  vector<pair<string, vector<string>>> routes;

  for (const auto& activity : route) {
    if (holds_alternative<BusActivity>(activity)) {
      auto bus_activity = get<BusActivity>(activity);
      const auto& bus = bus_dict_.at(bus_activity.bus());

      vector<string> bus_stops;
      copy_n(
        next(begin(bus->stops()), bus_activity.start_stop_idx()),
        bus_activity.span_count() + 1,
        back_inserter(bus_stops)
      );
      routes.emplace_back(bus->name(), bus_stops);
    }
  }

  BuildBusLines(doc, routes);
}

void MapBuilder::BuildBusLabels(Svg::Document &doc,
                                const std::vector<std::pair<std::string, std::vector<SvgPoint>>> &labels) const {
  auto create_bus_no_text = [&](string bus_no,
                                const SvgPoint &point) -> Text {
    return Text{}
             .SetPoint(point)
             .SetOffset(render_settings_.bus_label_offset())
             .SetFontSize(render_settings_.bus_label_font_size())
             .SetFontFamily("Verdana")
             .SetFontWeight("bold")
             .SetData(bus_no);
  };

  auto add_bus_label = [&](const string &label,
                           const SvgPoint &point,
                           const Color &stroke_color) {
    doc.Add(create_bus_no_text(label, point)
              .SetFillColor(render_settings_.underlayer_color())
              .SetStrokeColor(render_settings_.underlayer_color())
              .SetStrokeWidth(render_settings_.underlayer_width())
              .SetStrokeLineCap("round")
              .SetStrokeLineJoin("round"));
    doc.Add(create_bus_no_text(label, point).SetFillColor(stroke_color));
  };

  for (const auto &label : labels) {
    for (const auto &point : label.second) {
      add_bus_label(label.first, point, route_color.at(label.first));
    }
  }
}

void MapBuilder::BuildBusLabels(Svg::Document &doc) const {
  vector<pair<string, vector<SvgPoint>>> labels;
  for (const auto &[bus_no, bus] : bus_dict_) {
    vector<SvgPoint> points;

    const auto &first_stop = bus->stops(0);
    points.push_back(MapStop(first_stop));

    const auto last_stop = bus->stops(bus->stops_size() / 2);
    if (!bus->is_roundtrip() && first_stop != last_stop) {
      points.push_back(MapStop(last_stop));
    }

    labels.emplace_back(bus_no, points);
  }

  BuildBusLabels(doc, labels);
}

void MapBuilder::BuildBusLabelsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const {
  vector<pair<string, vector<SvgPoint>>> labels;

  for (const auto& activity : route) {
    if (holds_alternative<BusActivity>(activity)) {
      auto bus_activity = get<BusActivity>(activity);
      const auto& bus = bus_dict_.at(bus_activity.bus());

      vector<SvgPoint> points;

      auto endpoints = Endpoints(*bus);

      const auto &first_stop = bus->stops(bus_activity.start_stop_idx());
      if ((first_stop == endpoints.first) || (endpoints.second && first_stop == endpoints.second)) {
        points.push_back(MapStop(first_stop));
      }

      const auto &last_stop = bus->stops(bus_activity.start_stop_idx() + bus_activity.span_count());
      if ((endpoints.second && last_stop == endpoints.second) || (last_stop == endpoints.first)) {
        points.push_back(MapStop(last_stop));
      }

      labels.emplace_back(bus->name(), points);
    }
  }

  BuildBusLabels(doc, labels);
}

void MapBuilder::BuildStopPoints(Svg::Document &doc, const std::vector<std::string> &stop_names) const {
  for (const auto &stop_name : stop_names) {
    Color fill_color;
    fill_color.set_name("white");

    doc.Add(Circle{}
              .SetCenter(MapStop(stop_name))
              .SetRadius(render_settings_.stop_radius())
              .SetFillColor(fill_color)
    );
  }
}

void MapBuilder::BuildStopPoints(Svg::Document &doc) const {
  vector<string> stop_names;
  for (const auto &[stop_name, stop] : stop_dict_) {
    stop_names.push_back(stop_name);
  }
  BuildStopPoints(doc, stop_names);
}

void MapBuilder::BuildStopPointsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const {
  vector<string> stop_names;

  for (const auto& activity : route) {
    if (holds_alternative<BusActivity>(activity)) {
      auto bus_activity = get<BusActivity>(activity);
      const auto& bus = bus_dict_.at(bus_activity.bus());

      if (bus_activity.span_count() > 0) {
        copy_n(
          begin(bus->stops()) + bus_activity.start_stop_idx(),
          bus_activity.span_count() + 1,
          back_inserter(stop_names)
        );
      }
    }
  }

  BuildStopPoints(doc, stop_names);
}

void MapBuilder::BuildStopLabels(Svg::Document &doc, const std::vector<std::string> &stop_names) const {
  for (const auto &stop_name : stop_names) {
    const auto common = Text{}
                            .SetPoint(MapStop(stop_name))
                            .SetOffset(render_settings_.stop_label_offset())
                            .SetFontSize(render_settings_.stop_label_font_size())
                            .SetFontFamily("Verdana")
                            .SetData(stop_name);

    doc.Add(Text{common}
                .SetFillColor(render_settings_.underlayer_color())
                .SetStrokeColor(render_settings_.underlayer_color())
                .SetStrokeWidth(render_settings_.underlayer_width())
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round"));

    Color fill_color;
    fill_color.set_name("black");
    doc.Add(Text{common}.SetFillColor(fill_color));
  }
}

void MapBuilder::BuildStopLabels(Svg::Document &doc) const {
  vector<string> stop_names;
  for (const auto &[stop_name, stop] : stop_dict_) {
    stop_names.push_back(stop_name);
  }
  BuildStopLabels(doc, stop_names);
}

void MapBuilder::BuildStopLabelsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const {
  vector<string> stop_names;

  for (const auto& activity : route) {
    if (holds_alternative<WaitActivity>(activity)) {
      auto wait_activity = get<WaitActivity>(activity);
      stop_names.push_back(wait_activity.stop_name());
    }
  }

  if (route.size() > 1) {
    auto bus_activity = get<BusActivity>(route.back());
    auto last_stop = bus_dict_.at(bus_activity.bus())->stops(bus_activity.start_stop_idx() + bus_activity.span_count());
    stop_names.push_back(last_stop);
  }

  BuildStopLabels(doc, stop_names);
}

const std::unordered_map<std::string, void (MapBuilder::*)(Svg::Document &) const>
    MapBuilder::build = {
        {"bus_lines", &MapBuilder::BuildBusLines},
        {"bus_labels", &MapBuilder::BuildBusLabels},
        {"stop_points", &MapBuilder::BuildStopPoints},
        {"stop_labels", &MapBuilder::BuildStopLabels},
};

const std::unordered_map<std::string, void (MapBuilder::*)(Svg::Document &, const RouteDescription::Route &route) const>
    MapBuilder::build_route = {
        {"bus_lines", &MapBuilder::BuildBusLinesOnRoute},
        {"bus_labels",  &MapBuilder::BuildBusLabelsOnRoute},
        {"stop_points", &MapBuilder::BuildStopPointsOnRoute},
        {"stop_labels", &MapBuilder::BuildStopLabelsOnRoute},
};

}
