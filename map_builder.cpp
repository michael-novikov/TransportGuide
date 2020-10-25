#include "map_builder.h"

#include "transport_manager_command.h"
#include "stop.h"
#include "svg.h"

#include <algorithm>
#include <utility>

using namespace std;
using namespace Svg;

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

  auto [min_lon_it, max_lon_it] = minmax_element(begin(stops), end(stops),
      [](const Stop& lhs, const Stop& rhs) {
        return lhs.StopCoordinates().longitude < rhs.StopCoordinates().longitude;
      });

  auto min_lat = min_lat_it->StopCoordinates().latitude;
  auto max_lat = max_lat_it->StopCoordinates().latitude;
  auto min_lon = min_lon_it->StopCoordinates().longitude;
  auto max_lon = max_lon_it->StopCoordinates().longitude;

  double width_zoom_coef = max_lon - min_lon > 0 ?
    (render_settings_.width - 2 * render_settings_.padding) / (max_lon - min_lon) : 0;

  double height_zoom_coef = max_lat - min_lat > 0 ?
    (render_settings_.height - 2 * render_settings_.padding) / (max_lat - min_lat) : 0;

  double zoom_coef = min(width_zoom_coef, height_zoom_coef);

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

  // 2. Draw stop circles
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

  // 3. Draw stop names
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
