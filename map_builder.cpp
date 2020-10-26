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

MapBuilder::MapBuilder(RenderSettings render_settings)
  : render_settings_(move(render_settings))
{
}

double MapBuilder::ComputeZoomCoef(const std::vector<Stop>& stops) const {
  return 0;
}

std::string MapBuilder::BuildMap(const std::vector<Stop>& stops,
                                 const std::map<std::string, size_t>& stop_idx,
                                 const std::map<BusRoute::RouteNumber, BusRoute>& buses) const {
  Document doc;

  auto [min_lat_it, max_lat_it] = minmax_element(begin(stops), end(stops),
      [](const Stop& lhs, const Stop& rhs) {
        return lhs.StopCoordinates().latitude < rhs.StopCoordinates().latitude;
      });
  auto min_lat = min_lat_it->StopCoordinates().latitude;
  auto max_lat = max_lat_it->StopCoordinates().latitude;

  auto [min_lon_it, max_lon_it] = minmax_element(begin(stops), end(stops),
      [](const Stop& lhs, const Stop& rhs) {
        return lhs.StopCoordinates().longitude < rhs.StopCoordinates().longitude;
      });
  auto min_lon = min_lon_it->StopCoordinates().longitude;
  auto max_lon = max_lon_it->StopCoordinates().longitude;

  auto width_zoom_coef = [this](double min_lon, double max_lon) {
    return (render_settings_.width - 2 * render_settings_.padding) / (max_lon - min_lon);
  };
  auto height_zoom_coef = [this](double min_lat, double max_lat) {
    return (render_settings_.height - 2 * render_settings_.padding) / (max_lat - min_lat);
  };

  double zoom_coef;
  if ((abs(max_lon - min_lon) < EPS) && (abs(max_lat - min_lat) < EPS)) {
    zoom_coef = 0.0;
  } else if (abs(max_lon - min_lon) < EPS) {
    zoom_coef = height_zoom_coef(min_lat, max_lat);
  } else if (abs(max_lat - min_lat) < EPS) {
    zoom_coef = width_zoom_coef(min_lon, max_lon);
  } else {
    zoom_coef = min(width_zoom_coef(min_lon, max_lon), height_zoom_coef(min_lat, max_lat));
  }

  // 1. Draw route lines
  int color_index{0};
  for (const auto& [bus_no, bus] : buses) {
    const Color& stroke_color = render_settings_.color_palette[color_index];

    auto route_line = Polyline{}
      .SetStrokeColor(stroke_color)
      .SetStrokeWidth(render_settings_.line_width)
      .SetStrokeLineCap("round")
      .SetStrokeLineJoin("round");

    for (const auto& stop_name : bus.Stops()) {
      const auto &coordinates = stops[stop_idx.at(stop_name)].StopCoordinates();
      route_line.AddPoint({
          (coordinates.longitude - min_lon) * zoom_coef + render_settings_.padding,
          (max_lat - coordinates.latitude) * zoom_coef + render_settings_.padding,
      });
    }

    doc.Add(route_line);
    color_index = (color_index + 1) % render_settings_.color_palette.size();
  }

  // 2. Draw route numbers
  int route_no_color_index{0};
  for (const auto& [bus_no, bus] : buses) {
    const Color &stroke_color = render_settings_.color_palette[route_no_color_index];

    auto create_bus_no_text = [&](string bus_no, const Stop& stop) -> Text {
      const auto &coordinates = stop.StopCoordinates();

      return Text{}
          .SetPoint({
            (coordinates.longitude - min_lon) * zoom_coef + render_settings_.padding,
            (max_lat - coordinates.latitude) * zoom_coef + render_settings_.padding,
          })
          .SetOffset(render_settings_.bus_label_offset)
          .SetFontSize(render_settings_.bus_label_font_size)
          .SetFontFamily("Verdana")
          .SetFontWeight("bold")
          .SetData(bus_no);
    };

    const auto& first_stop = stops[stop_idx.at(bus.Stops().front())];
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

    const auto last_stop_idx = bus.Stops().size() / 2;
    if (!bus.IsRoundTrip() && bus.Stops()[0] != bus.Stops()[last_stop_idx]) {
      const auto& last_stop = stops[stop_idx.at(bus.Stops()[last_stop_idx])];
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

  // 3. Draw stop circles
  for (const auto& [stop_name, stop_id] : stop_idx) {
    const auto &coordinates = stops[stop_idx.at(stop_name)].StopCoordinates();

    doc.Add(
      Circle{}
      .SetCenter({
          (coordinates.longitude - min_lon) * zoom_coef + render_settings_.padding,
          (max_lat - coordinates.latitude) * zoom_coef + render_settings_.padding,
      })
      .SetRadius(render_settings_.stop_radius)
      .SetFillColor("white")
    );
  }

  // 4. Draw stop names
  for (const auto& [stop_name, stop_id] : stop_idx) {
    const auto &coordinates = stops[stop_idx.at(stop_name)].StopCoordinates();

    const auto common =
      Text{}
      .SetPoint({
          (coordinates.longitude - min_lon) * zoom_coef + render_settings_.padding,
          (max_lat - coordinates.latitude) * zoom_coef + render_settings_.padding,
      })
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

  return doc.ToString();
}
