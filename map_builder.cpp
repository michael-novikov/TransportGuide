#include "map_builder.h"

#include "stop.h"
#include "svg.h"
#include "transport_manager_command.h"

#include "projector_interface.h"
#include "scanline_compressed_projection.h"
#include "scanline_projection.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iterator>
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
    : render_settings_(move(render_settings))
    , stop_idx_(stop_idx)
    , buses_(buses)
    , stop_coordinates_(ComputeStopsCoords(render_settings_, stops, stop_idx_, buses))
{
  int color_idx{0};
  for (const auto &[bus_no, bus] : buses_) {
    route_color[bus_no] = render_settings_.color_palette[color_idx];
    color_idx = (color_idx + 1) % render_settings_.color_palette.size();
  }

  doc_ = BuildMap();
}

Svg::Document MapBuilder::BuildMap() const {
  Svg::Document doc;

  for (const auto &layer : render_settings_.layers) {
    (this->*build.at(layer))(doc);
  }

  return doc;
}

void MapBuilder::BuildTranslucentRoute(Svg::Document &doc) const {
  doc.Add(
    Rectangle{}
      .SetBasePoint({-render_settings_.outer_margin, -render_settings_.outer_margin})
      .SetWidth(render_settings_.width + 2 * render_settings_.outer_margin)
      .SetHeight(render_settings_.height + 2 * render_settings_.outer_margin)
      .SetFillColor(render_settings_.underlayer_color)
  );
}

Svg::Document MapBuilder::BuildRoute(const RouteInfo::Route &route) const {
  auto res = BuildMap();
  BuildTranslucentRoute(res);

  for (const auto &layer : render_settings_.layers) {
    (this->*build_route.at(layer))(res, route);
  }

  return res;
}

Svg::Point MapBuilder::MapStop(const std::string &stop_name) const {
  return stop_coordinates_.at(stop_name);
}

void MapBuilder::BuildBusLines(Svg::Document &doc,
    const std::vector<std::pair<std::string, std::vector<std::string>>> &lines) const {
  for (const auto &line : lines) {
    auto route_line = Polyline{}
                          .SetStrokeColor(route_color.at(line.first))
                          .SetStrokeWidth(render_settings_.line_width)
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
  for (const auto &[bus_no, bus] : buses_) {
    full_routes.emplace_back(bus_no, bus.Stops());
  }
  BuildBusLines(doc, full_routes);
}

void MapBuilder::BuildBusLinesOnRoute(Svg::Document &doc, const RouteInfo::Route &route) const {
  vector<pair<string, vector<string>>> routes;

  for (const auto& activity : route) {
    if (holds_alternative<BusActivity>(activity)) {
      auto bus_activity = get<BusActivity>(activity);
      const auto& bus = buses_.at(bus_activity.bus);

      vector<string> bus_stops;
      copy_n(
        next(begin(bus.Stops()), bus_activity.start_stop_idx),
        bus_activity.span_count + 1,
        back_inserter(bus_stops)
      );
      routes.emplace_back(bus.Number(), bus_stops);
    }
  }

  BuildBusLines(doc, routes);
}

void MapBuilder::BuildBusLabels(Svg::Document &doc,
                                const std::vector<std::pair<std::string, std::vector<Svg::Point>>> &labels) const {
  auto create_bus_no_text = [&](string bus_no,
                                const Svg::Point &point) -> Text {
    return Text{}
             .SetPoint(point)
             .SetOffset(render_settings_.bus_label_offset)
             .SetFontSize(render_settings_.bus_label_font_size)
             .SetFontFamily("Verdana")
             .SetFontWeight("bold")
             .SetData(bus_no);
  };

  auto add_bus_label = [&](const string &label,
                           const Svg::Point &point,
                           const Color &stroke_color) {
    doc.Add(create_bus_no_text(label, point)
              .SetFillColor(render_settings_.underlayer_color)
              .SetStrokeColor(render_settings_.underlayer_color)
              .SetStrokeWidth(render_settings_.underlayer_width)
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
  vector<pair<string, vector<Svg::Point>>> labels;
  for (const auto &[bus_no, bus] : buses_) {
    vector<Svg::Point> points;

    const auto &first_stop = bus.Stops().front();
    points.push_back(MapStop(first_stop));

    const auto last_stop = bus.Stop(bus.Stops().size() / 2);
    if (!bus.IsRoundTrip() && first_stop != last_stop) {
      points.push_back(MapStop(last_stop));
    }

    labels.emplace_back(bus_no, points);
  }

  BuildBusLabels(doc, labels);
}

void MapBuilder::BuildBusLabelsOnRoute(Svg::Document& doc, const RouteInfo::Route &route) const {
  vector<pair<string, vector<Svg::Point>>> labels;

  for (const auto& activity : route) {
    if (holds_alternative<BusActivity>(activity)) {
      auto bus_activity = get<BusActivity>(activity);
      const auto& bus = buses_.at(bus_activity.bus);

      vector<Svg::Point> points;

      auto endpoints = bus.Endpoints();

      const auto &first_stop = bus.Stop(bus_activity.start_stop_idx);
      if ((first_stop == endpoints.first) || (endpoints.second && first_stop == endpoints.second)) {
        points.push_back(MapStop(first_stop));
      }

      const auto &last_stop = bus.Stop(bus_activity.start_stop_idx + bus_activity.span_count);
      if ((endpoints.second && last_stop == endpoints.second) || (last_stop == endpoints.first)) {
        points.push_back(MapStop(last_stop));
      }

      labels.emplace_back(bus.Number(), points);
    }
  }

  BuildBusLabels(doc, labels);
}

void MapBuilder::BuildStopPoints(Svg::Document &doc, const std::vector<std::string> &stop_names) const {
  for (const auto &stop_name : stop_names) {
    doc.Add(Circle{}
              .SetCenter(MapStop(stop_name))
              .SetRadius(render_settings_.stop_radius)
              .SetFillColor("white")
    );
  }
}

void MapBuilder::BuildStopPoints(Svg::Document &doc) const {
  vector<string> stop_names;
  for (const auto &[stop_name, stop_id] : stop_idx_) {
    stop_names.push_back(stop_name);
  }
  BuildStopPoints(doc, stop_names);
}

void MapBuilder::BuildStopPointsOnRoute(Svg::Document& doc, const RouteInfo::Route &route) const {
  vector<string> stop_names;

  for (const auto& activity : route) {
    if (holds_alternative<BusActivity>(activity)) {
      auto bus_activity = get<BusActivity>(activity);
      const auto& bus = buses_.at(bus_activity.bus);

      if (bus_activity.span_count > 0) {
        copy_n(
          begin(bus.Stops()) + bus_activity.start_stop_idx,
          bus_activity.span_count + 1,
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

void MapBuilder::BuildStopLabels(Svg::Document &doc) const {
  vector<string> stop_names;
  for (const auto &[stop_name, stop_id] : stop_idx_) {
    stop_names.push_back(stop_name);
  }
  BuildStopLabels(doc, stop_names);
}

void MapBuilder::BuildStopLabelsOnRoute(Svg::Document& doc, const RouteInfo::Route &route) const {
  vector<string> stop_names;

  for (const auto& activity : route) {
    if (holds_alternative<WaitActivity>(activity)) {
      auto wait_activity = get<WaitActivity>(activity);
      stop_names.push_back(wait_activity.stop_name);
    }
  }

  if (route.size() > 1) {
    auto bus_activity = get<BusActivity>(route.back());
    auto last_stop = buses_.at(bus_activity.bus).Stop(bus_activity.start_stop_idx + bus_activity.span_count);
    stop_names.push_back(last_stop);
  }

  BuildStopLabels(doc, stop_names);
}

const std::unordered_map<MapLayer, void (MapBuilder::*)(Svg::Document &) const>
    MapBuilder::build = {
        {MapLayer::BUS_LINES, &MapBuilder::BuildBusLines},
        {MapLayer::BUS_LABELS, &MapBuilder::BuildBusLabels},
        {MapLayer::STOP_POINTS, &MapBuilder::BuildStopPoints},
        {MapLayer::STOP_LABELS, &MapBuilder::BuildStopLabels},
};

const std::unordered_map<MapLayer, void (MapBuilder::*)(Svg::Document &, const RouteInfo::Route &route) const>
    MapBuilder::build_route = {
        {MapLayer::BUS_LINES, &MapBuilder::BuildBusLinesOnRoute},
        {MapLayer::BUS_LABELS, &MapBuilder::BuildBusLabelsOnRoute},
        {MapLayer::STOP_POINTS, &MapBuilder::BuildStopPointsOnRoute},
        {MapLayer::STOP_LABELS, &MapBuilder::BuildStopLabelsOnRoute},
};
